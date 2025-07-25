#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <algorithm>
#include <array>
#include <string>

bool validate_pw(const std::array<unsigned int, 8>& code) {
  bool test_passed = false;
  for (size_t i = 0; i < code.size() - 2; ++i) {
    if (code[i] + 1 == code[i + 1] && code[i + 1] + 1 == code[i + 2]) {
      test_passed = true;
      break;
    }
  }
  if (!test_passed) {
    return false;
  }
  test_passed = !std::any_of(
      code.begin(), code.end(), [](unsigned int n) { return n == 9 || n == 12 || n == 15; });
  if (!test_passed) {
    return false;
  }
  int pairCount = 0;
  for (size_t i = 0; i < code.size() - 1; ++i) {
    if (code[i] == code[i + 1]) {
      ++pairCount;
      ++i;
    }
  }
  test_passed = pairCount >= 2;
  return test_passed;
}

void increment_pw(std::array<unsigned int, 8>& code) {
  size_t i, n;
  unsigned int new_digit;
  bool done = false;
  n = code.size();
  i = n - 1;
  while (!done) {
    new_digit = std::max<unsigned int>((code[i] + 1) % 27, 1);
    done = new_digit != 1;
    code[i] = new_digit;
    if (i == 0) {
      i = n - 1;
    } else {
      i--;
    }
  }
}

// [[Rcpp::export]]
std::string find_next_password(std::string pw) {
  std::array<unsigned int, 8> code;
  size_t i = 0;
  for (auto const& c : pw) {
    code[i++] = static_cast<char>(c) - 'a' + 1;
  }
  bool done = false;
  i = 0;
  while (!done) {
    increment_pw(code);
    done = validate_pw(code);
    i++;
  }
  auto stringify {[](const std::array<unsigned int, 8>& code) {
    std::string result;
    for (auto d : code) {
      char letter = 'a' + (d - 1);
      result += letter;
    }
    return result;
  }};
  return stringify(code);
}

#ifdef STANDALONE
int main() {
  std::string password = "ghijklmn";
  std::string next_password = find_next_password(password);
  std::cout << "Original password: " << password << " Next password: " << next_password
            << std::endl;
  return 0;
}
#endif // STANDALONE