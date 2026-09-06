"""Maintainer checks for scoring, report integrity and failure handling."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest import mock

REPO_ROOT = Path(__file__).resolve().parents[3]
TEST_DIR = REPO_ROOT / "test_files" / "data"
spec = importlib.util.spec_from_file_location("grader", REPO_ROOT / "eval" / "test_by_code.py")
grader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(grader)
allocation_spec = importlib.util.spec_from_file_location(
    "allocation_checker", TEST_DIR / "task-1-gol" / "check_no_allocation.py")
allocation_checker = importlib.util.module_from_spec(allocation_spec)
allocation_spec.loader.exec_module(allocation_checker)


class GraderTests(unittest.TestCase):
    def setUp(self):
        self.config = grader.load_config(TEST_DIR / "grading.json")
        self.outcomes = {name: {"statuses": {group: True for group in task["groups"]}, "error": None}
                         for name, task in self.config["tasks"].items()}
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.addCleanup(self.temp.cleanup)

    def test_all_pass_exact_schema(self):
        self.assertEqual(grader.aggregate(self.config, self.outcomes),
                         {"resolved": True, "score": 1.0, "reason": "all tests passed"})

    def test_no_pass(self):
        for outcome in self.outcomes.values():
            outcome["statuses"] = dict.fromkeys(outcome["statuses"], False)
        result = grader.aggregate(self.config, self.outcomes)
        self.assertFalse(result["resolved"])
        self.assertEqual(result["score"], 0.0)
        self.assertIn("passed 0/22", result["reason"])

    def test_threshold_boundary(self):
        for group in ("transformation.translate", "transformation.rotate",
                      "transformation.lookat"):
            self.outcomes["task-2-transformation"]["statuses"][group] = False
        result = grader.aggregate(self.config, self.outcomes)
        self.assertEqual(result["score"], 0.8)
        self.assertTrue(result["resolved"])
        self.outcomes["task-1-gol"]["statuses"]["gol.nonbinary"] = False
        result = grader.aggregate(self.config, self.outcomes)
        self.assertEqual(result["score"], 0.775)
        self.assertFalse(result["resolved"])

    def test_one_build_error_preserves_other_tasks(self):
        self.outcomes["task-2-transformation"] = {"statuses": {}, "error": "build failed"}
        result = grader.aggregate(self.config, self.outcomes)
        self.assertEqual(result["score"], 0.75)
        self.assertFalse(result["resolved"])
        self.assertIn("build failed", result["reason"])

    def test_allocation_failure_caps_score_but_preserves_resolution(self):
        self.outcomes["task-1-gol"]["statuses"]["gol.no_allocation"] = False
        result = grader.aggregate(self.config, self.outcomes)
        self.assertEqual(result["score"], 0.6)
        self.assertTrue(result["resolved"])
        self.assertIn("score capped at 0.6 because gol.no_allocation failed", result["reason"])

    def test_c_allocation_source_check_ignores_comments_and_literals(self):
        harmless = r'''// malloc(10), new int[20], std::vector<int>
const char *message = "calloc(2, 3)";
/* realloc(pointer, 20); int scratch[100]; */
int allocation_count = 0;
'''
        self.assertIsNone(allocation_checker.find_forbidden(harmless))
        for name in ("malloc", "calloc", "realloc", "aligned_alloc", "alloca"):
            with self.subTest(name=name):
                self.assertEqual(allocation_checker.find_forbidden(
                    "void *memory = std::{}(16);".format(name)), name)
        self.assertEqual(allocation_checker.find_forbidden("auto *p = new int[20];"), "new")
        self.assertEqual(allocation_checker.find_forbidden("std::vector<int> next;"),
                         "standard container")
        self.assertEqual(allocation_checker.find_forbidden("uint8_t next[width * height];"),
                         "auxiliary array")

    def test_missing_internal_test_is_not_a_pass(self):
        del self.outcomes["task-1-gol"]["statuses"]["gol.patterns"]
        with self.assertRaises(grader.EvaluationError):
            grader.aggregate(self.config, self.outcomes)

    def test_weight_validation(self):
        for invalid in (0, -1, True, 0.25):
            config = copy.deepcopy(self.config)
            config["tasks"]["task-1-gol"]["weight"] = invalid
            path = self.root / "config.json"
            path.write_text(json.dumps(config))
            with self.assertRaises(grader.EvaluationError):
                grader.load_config(path)

    def test_group_weight_and_score_cap_validation(self):
        for invalid in (-1, True, 0.25):
            config = copy.deepcopy(self.config)
            config["tasks"]["task-1-gol"]["groups"]["gol.no_allocation"] = invalid
            path = self.root / "config.json"
            path.write_text(json.dumps(config))
            with self.subTest(group_weight=invalid), self.assertRaises(grader.EvaluationError):
                grader.load_config(path)
        for invalid in (-0.1, 1.1, True, float("nan")):
            config = copy.deepcopy(self.config)
            config["score_caps"]["gol.no_allocation"] = invalid
            path = self.root / "config.json"
            path.write_text(json.dumps(config))
            with self.subTest(score_cap=invalid), self.assertRaises(grader.EvaluationError):
                grader.load_config(path)
        config = copy.deepcopy(self.config)
        config["score_caps"] = {"unknown.test": 0.6}
        path = self.root / "config.json"
        path.write_text(json.dumps(config))
        with self.assertRaises(grader.EvaluationError):
            grader.load_config(path)

    def test_result_replacement_and_types(self):
        path = self.root / "eval" / "code_result.json"
        grader.write_result(path, {"resolved": True, "score": 1.0, "reason": "old result"})
        failure = {"resolved": False, "score": 0.0, "reason": "编译失败"}
        grader.write_result(path, failure)
        self.assertEqual(json.loads(path.read_text()), failure)
        self.assertEqual(list(path.parent.iterdir()), [path])
        for changes in ({"extra": 1}, {"score": float("nan")}, {"score": 1.1},
                        {"score": -0.1}, {"score": True}, {"resolved": 1}, {"reason": []}):
            with self.subTest(changes=changes), self.assertRaises(grader.EvaluationError):
                grader.write_result(path, {**failure, **changes})

    def report(self, tests):
        testing = self.root / "Testing"
        folder = testing / "20260906-1200"
        folder.mkdir(parents=True, exist_ok=True)
        (testing / "TAG").write_text("20260906-1200\nExperimental\n")
        path = folder / "Test.xml"
        path.write_text("<Site><Testing>" + tests + "</Testing></Site>")
        return path

    def test_xml_failed_and_notrun_get_no_credit(self):
        self.report('<Test Status="passed"><Name>a</Name></Test>'
                    '<Test Status="failed"><Name>b</Name></Test>'
                    '<Test Status="notrun"><Name>c</Name></Test>')
        self.assertEqual(grader.parse_ctest_results(self.root, ["a", "b", "c"]),
                         {"a": True, "b": False, "c": False})

    def test_incomplete_duplicate_and_unknown_xml_rejected(self):
        for xml in ("", '<Test Status="passed"><Name>unknown</Name></Test>',
                    '<Test Status="bogus"><Name>a</Name></Test>',
                    '<Test Status="passed"><Name>a</Name></Test>' * 2):
            self.report(xml)
            with self.subTest(xml=xml), self.assertRaises(grader.EvaluationError):
                grader.parse_ctest_results(self.root, ["a"])

    def test_missing_or_malformed_xml_rejected(self):
        with self.assertRaises(grader.EvaluationError):
            grader.parse_ctest_results(self.root, ["a"])
        self.report("").write_text("not xml")
        with self.assertRaises(grader.EvaluationError):
            grader.parse_ctest_results(self.root, ["a"])

    def test_report_path_traversal_rejected(self):
        self.report("")
        (self.root / "Testing" / "TAG").write_text("../../somewhere\n")
        with self.assertRaises(grader.EvaluationError):
            grader.parse_ctest_results(self.root, ["a"])

    def test_real_command_exit_and_log(self):
        log = self.root / "run.log"
        code = grader.run_command([sys.executable, "-c", "print('diagnostic'); raise SystemExit(7)"],
                                  self.root, log, 5)
        self.assertEqual(code, 7)
        self.assertIn("diagnostic", log.read_text())

    def test_real_command_timeout(self):
        start = time.monotonic()
        with self.assertRaisesRegex(grader.EvaluationError, "timed out"):
            grader.run_command([sys.executable, "-c", "import time; time.sleep(60)"],
                               self.root, self.root / "timeout.log", 0.2)
        self.assertLess(time.monotonic() - start, 5)

    def test_missing_executable(self):
        with self.assertRaisesRegex(grader.EvaluationError, "cannot start"):
            grader.run_command([str(self.root / "does-not-exist")], self.root, self.root / "missing.log", 5)

    def test_build_failure_classified(self):
        with mock.patch.object(grader, "run_command", side_effect=[0, 2]):
            result = grader.evaluate_task("task-1-gol", self.config["tasks"]["task-1-gol"], self.config,
                                          TEST_DIR, self.root / "build", self.root)
        self.assertIn("build failed", result["error"])
        self.assertEqual(result["statuses"], {})

    def test_ctest_report_and_exit_disagreement(self):
        with mock.patch.object(grader, "run_command", side_effect=[0, 0, 8]), \
             mock.patch.object(grader, "parse_ctest_results", return_value=self.outcomes["task-1-gol"]["statuses"]):
            result = grader.evaluate_task("task-1-gol", self.config["tasks"]["task-1-gol"], self.config,
                                          TEST_DIR, self.root / "build", self.root)
        self.assertIn("disagree", result["error"])

    def test_main_uses_eval_working_directory_and_continues_after_task_failure(self):
        tests = self.root / "test_files" / "data"
        tests.parent.mkdir()
        tests.mkdir()
        evaluation = self.root / "eval"
        evaluation.mkdir()
        (tests / "grading.json").write_text(json.dumps(self.config))
        outcomes = list(self.outcomes.values())
        outcomes[0] = {"statuses": {}, "error": "configure failed"}
        with mock.patch.object(grader.Path, "cwd", return_value=evaluation), \
             mock.patch.object(grader, "evaluate_task", side_effect=outcomes) as evaluate, \
             mock.patch("builtins.print"):
            self.assertEqual(grader.main(), 0)
            self.assertEqual(evaluate.call_count, 4)
        result = json.loads((evaluation / "code_result.json").read_text())
        self.assertEqual(set(result), {"resolved", "score", "reason"})
        self.assertEqual(result["score"], 0.75)
        self.assertFalse(result["resolved"])

    def test_bad_config_overwrites_stale_success(self):
        tests = self.root / "test_files" / "data"
        tests.parent.mkdir()
        tests.mkdir()
        evaluation = self.root / "eval"
        evaluation.mkdir()
        (tests / "grading.json").write_text("not json")
        path = evaluation / "code_result.json"
        grader.write_result(path, {"resolved": True, "score": 1.0, "reason": "old success"})
        with mock.patch.object(grader.Path, "cwd", return_value=evaluation), mock.patch("builtins.print"):
            self.assertEqual(grader.main(), 0)
        result = json.loads(path.read_text())
        self.assertFalse(result["resolved"])
        self.assertEqual(result["score"], 0.0)
        self.assertIn("evaluation error", result["reason"])


if __name__ == "__main__":
    unittest.main()
