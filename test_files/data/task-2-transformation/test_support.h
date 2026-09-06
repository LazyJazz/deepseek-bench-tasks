#pragma once
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + #__VA_ARGS__); } while (false)
template<class F> int Run(F f) { try { f(); std::cout << "PASS\n"; return 0; } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
