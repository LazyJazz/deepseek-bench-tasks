#pragma once
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(   std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__); } while (false)
inline std::ifstream Data(const std::string &name, bool binary = false) {
  std::ifstream in(std::string(TEST_DATA_DIR) + "/" + name,
                   binary ? std::ios::in | std::ios::binary : std::ios::in);
  CHECK(in.is_open());
  return in;
}
template<class F> int Run(F f) {
  try { f(); std::cout << "PASS\n"; return 0; }
  catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
