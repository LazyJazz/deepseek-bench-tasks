#!/usr/bin/env python3
"""Run from eval/ and write exactly three fields to code_result.json.

Python 3.8+ standard library only. A zero exit code means a result was written;
read `resolved` for the benchmark verdict, including compilation failures.
"""
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


class EvaluationError(RuntimeError):
    pass


def load_config(path):
    with path.open(encoding="utf-8") as stream:
        config = json.load(stream)
    threshold = config["resolved_threshold"]
    if isinstance(threshold, bool) or not isinstance(threshold, (int, float)) or not 0 < threshold <= 1:
        raise EvaluationError("resolved_threshold must be in (0, 1]")
    tasks = config["tasks"]
    expected_tasks = {"task-1-gol", "task-2-transformation",
                      "task-3-rasterization", "task-4-raytracing"}
    if set(tasks) != expected_tasks:
        raise EvaluationError("grading configuration has an unexpected task set")
    for task in tasks.values():
        if type(task["weight"]) is not int or task["weight"] <= 0:
            raise EvaluationError("task weights must be positive integers")
        group_weights = list(task["groups"].values())
        if (not group_weights or any(type(weight) is not int or weight < 0 for weight in group_weights)
                or sum(group_weights) == 0):
            raise EvaluationError("group weights must be non-negative integers with a positive sum")
    all_groups = {group for task in tasks.values() for group in task["groups"]}
    for group, cap in config.get("score_caps", {}).items():
        if group not in all_groups:
            raise EvaluationError("score cap references an unknown test group: " + group)
        if (isinstance(cap, bool) or not isinstance(cap, (int, float))
                or not math.isfinite(cap) or not 0 <= cap <= 1):
            raise EvaluationError("score caps must be finite numbers between zero and one")
    for key in ("configure_timeout_seconds", "build_timeout_seconds"):
        if type(config[key]) is not int or config[key] <= 0:
            raise EvaluationError("phase timeouts must be positive integers")
    return config


def write_result(path, result):
    """Atomic replacement prevents old successful results surviving a failed run."""
    if set(result) != {"resolved", "score", "reason"}:
        raise EvaluationError("result must contain exactly resolved, score, reason")
    if type(result["resolved"]) is not bool or not isinstance(result["reason"], str):
        raise EvaluationError("invalid result field types")
    score = result["score"]
    if isinstance(score, bool) or not isinstance(score, (int, float)) or not math.isfinite(score) or not 0 <= score <= 1:
        raise EvaluationError("score must be a finite number between zero and one")
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=".code-result-", suffix=".tmp", delete=False) as stream:
            temp_name = stream.name
            json.dump(result, stream, ensure_ascii=False, allow_nan=False, indent=2)
            stream.write("\n")
        os.replace(temp_name, path)
    finally:
        if temp_name and os.path.exists(temp_name):
            os.unlink(temp_name)


def stop_process_tree(process):
    if os.name == "posix":
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    else:
        # taskkill is supplied by Windows; /T includes compiler/test child processes.
        try:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(process.pid)],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
        except (OSError, subprocess.TimeoutExpired):
            process.kill()
    process.wait()


def run_command(command, cwd, log_path, timeout):
    """No shell; bound wall time and stream output to disk instead of memory."""
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as log:
        log.write("Command: " + json.dumps(command) + "\n")
        log.flush()
        try:
            process = subprocess.Popen(command, cwd=str(cwd), stdout=log, stderr=subprocess.STDOUT,
                                       stdin=subprocess.DEVNULL, start_new_session=os.name == "posix")
        except OSError as error:
            raise EvaluationError("cannot start {}: {}".format(command[0], error)) from error
        try:
            return process.wait(timeout=timeout)
        except subprocess.TimeoutExpired as error:
            stop_process_tree(process)
            raise EvaluationError("timed out after {} seconds".format(timeout)) from error
        except BaseException:
            stop_process_tree(process)
            raise


def parse_ctest_results(build_dir, expected_names):
    """Use CTest's machine-readable XML, never human-readable console summaries."""
    try:
        tag = (build_dir / "Testing" / "TAG").read_text(encoding="utf-8").splitlines()[0]
        if not re.fullmatch(r"[0-9]+-[0-9]+", tag):
            raise EvaluationError("invalid CTest report tag")
        root = ET.parse(build_dir / "Testing" / tag / "Test.xml").getroot()
    except (OSError, IndexError, ET.ParseError) as error:
        raise EvaluationError("missing or malformed CTest report: {}".format(error)) from error
    statuses = {}
    for test in root.findall("./Testing/Test"):
        name = test.findtext("Name")
        status = test.get("Status")
        if name in statuses or name not in expected_names or status not in ("passed", "failed", "notrun"):
            raise EvaluationError("unexpected, duplicate, or invalid CTest result: {}".format(name))
        statuses[name] = status == "passed"
    if set(statuses) != set(expected_names):
        raise EvaluationError("CTest report does not contain the complete expected test set")
    return statuses


def evaluate_task(task_name, task, config, test_dir, build_dir, eval_dir):
    logs = eval_dir / task_name
    phase = "configure"
    try:
        code = run_command(["cmake", "-S", str(test_dir), "-B", str(build_dir),
                            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON", "-DBENCH_TASK=" + task_name],
                           test_dir, logs / "configure.log", config["configure_timeout_seconds"])
        if code != 0:
            raise EvaluationError("exited with code {}".format(code))
        phase = "build"
        code = run_command(["cmake", "--build", str(build_dir), "--config", "Release", "--parallel", "2"],
                           test_dir, logs / "build.log", config["build_timeout_seconds"])
        if code != 0:
            raise EvaluationError("exited with code {}".format(code))
        phase = "test"
        code = run_command(["ctest", "--test-dir", str(build_dir), "-C", "Release", "-T", "Test",
                            "--no-compress-output", "--output-on-failure", "--parallel", "1"],
                           test_dir, logs / "test.log", len(task["groups"]) * 35 + 30)
        # CTest uses 8 for ordinary test failures, including individual test timeouts.
        if code not in (0, 8):
            raise EvaluationError("CTest exited with code {}".format(code))
        statuses = parse_ctest_results(build_dir, task["groups"])
        if (code == 0) != all(statuses.values()):
            raise EvaluationError("CTest exit status and XML verdicts disagree")
        return {"statuses": statuses, "error": None}
    except (EvaluationError, OSError) as error:
        return {"statuses": {}, "error": "{} failed: {} (see eval/{}/{}.log)".format(
            phase, error, task_name, phase)}


def aggregate(config, outcomes):
    total_weight = sum(task["weight"] for task in config["tasks"].values())
    weighted_score = 0.0
    passed_count = 0
    total_count = 0
    failures = []
    for name, task in config["tasks"].items():
        total_count += len(task["groups"])
        outcome = outcomes.get(name, {"statuses": {}, "error": "evaluation did not run"})
        if outcome["error"]:
            failures.append(name + ": " + outcome["error"])
            continue
        statuses = outcome["statuses"]
        if set(statuses) != set(task["groups"]) or any(type(value) is not bool for value in statuses.values()):
            raise EvaluationError("invalid internal test outcome for " + name)
        passed_count += sum(statuses.values())
        points = sum(weight for group, weight in task["groups"].items() if statuses[group])
        weighted_score += task["weight"] * points / sum(task["groups"].values())
        failed = [group for group in task["groups"] if not statuses[group]]
        if failed:
            failures.append(name + ": failed " + ", ".join(failed))
    functional_score = weighted_score / total_weight
    resolved = functional_score >= config["resolved_threshold"]
    score = functional_score
    applied_caps = []
    for group, cap in config.get("score_caps", {}).items():
        failed = any(group in outcome["statuses"] and not outcome["statuses"][group]
                     for outcome in outcomes.values())
        if failed and score > cap:
            score = cap
            applied_caps.append("score capped at {} because {} failed".format(cap, group))
    score = round(score, 6)
    details = failures + applied_caps
    reason = "all tests passed" if not details else "passed {}/{} test groups; {}".format(
        passed_count, total_count, "; ".join(details))
    return {"resolved": resolved, "score": score, "reason": reason}


def main():
    # The benchmark runner sets eval/ as the working directory. Resolve every
    # repository path from that contract instead of from the script location.
    eval_dir = Path.cwd().resolve()
    test_dir = eval_dir.parent / "test_files" / "data"
    result_path = eval_dir / "code_result.json"
    result = {"resolved": False, "score": 0.0, "reason": "evaluation did not complete"}
    try:
        write_result(result_path, result)
        config = load_config(test_dir / "grading.json")
        outcomes = {}
        # Every run uses fresh build directories; candidate build files are never loaded.
        with tempfile.TemporaryDirectory(prefix="build-grading-", dir=eval_dir) as temp:
            for name, task in config["tasks"].items():
                outcomes[name] = evaluate_task(name, task, config, test_dir, Path(temp) / name, eval_dir)
        result = aggregate(config, outcomes)
    except KeyboardInterrupt:
        result = {"resolved": False, "score": 0.0, "reason": "evaluation interrupted"}
    except Exception as error:
        result = {"resolved": False, "score": 0.0, "reason": "evaluation error: {}: {}".format(type(error).__name__, error)}
    try:
        write_result(result_path, result)
    except Exception as error:
        print("Unable to write evaluation result: {}".format(error), file=sys.stderr)
        return 1
    print(json.dumps(result, ensure_ascii=False, allow_nan=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
