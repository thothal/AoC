#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using Key = std::variant<std::pair<int, int>, std::string>;

struct KeyHash {
    size_t operator()(const std::variant<std::pair<int, int>, std::string>& k) const {
      return std::visit(
          [](const auto& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, std::string>) {
              return std::hash<std::string>()(v);
            } else {
              size_t h1 = std::hash<int>()(v.first);
              size_t h2 = std::hash<int>()(v.second);
              return h1 ^ (h2 << 1);
            }
          },
          k);
    }
};

std::vector<bool> cyk_parse(const std::vector<std::string>& words,
                            const std::unordered_map<Key, std::vector<int>, KeyHash>& inverse_rules,
                            int start_symbol) {
  std::vector<bool> results;
  results.reserve(words.size());

  for (const auto& word : words) {
    size_t n = word.size();
    std::vector<std::vector<std::unordered_set<int>>> P(n, std::vector<std::unordered_set<int>>(n));
    for (size_t i = 0; i < n; ++i) {
      Key key = std::string(1, word[i]);
      if (auto it = inverse_rules.find(key); it != inverse_rules.end()) {
        P[i][0].insert(it->second.begin(), it->second.end());
      }
    }

    for (std::size_t j = 1; j < n; ++j) { // j = length of span - 1
      for (std::size_t i = 0; i + j < n; ++i) { // start of span
        for (std::size_t k = 0; k < j; ++k) { // partition of span
          const auto& left = P[i][k];
          const auto& right = P[i + k + 1][j - k - 1];
          if (left.empty() || right.empty()) {
            continue;
          }
          for (int B : left) {
            for (int C : right) {
              Key pair_key = std::make_pair(B, C);
              if (auto it = inverse_rules.find(pair_key); it != inverse_rules.end()) {
                P[i][j].insert(it->second.begin(), it->second.end());
              }
            }
          }
        }
      }
    }
    results.push_back(P[0][n - 1].count(start_symbol) > 0);
  }

  return results;
}

#ifndef STANDALONE
// [[Rcpp::export]]
LogicalVector is_member(List inv_rules, CharacterVector words, int start_symbol) {
  std::unordered_map<Key, std::vector<int>, KeyHash> inverse_rules;
  CharacterVector names = inv_rules.names();
  for (int i = 0; i < inv_rules.size(); ++i) {
    std::string key_str = as<std::string>(names[i]);
    IntegerVector rule = inv_rules[i];
    if (key_str.find('|') != std::string::npos) {
      size_t pipe_pos = key_str.find('|');
      int first = std::stoi(key_str.substr(0, pipe_pos));
      int second = std::stoi(key_str.substr(pipe_pos + 1));
      Key key = std::make_pair(first, second);
      inverse_rules[key] = as<std::vector<int>>(rule);
    } else {
      Key key = key_str;
      inverse_rules[key] = as<std::vector<int>>(rule);
    }
  }

  std::vector<std::string> word_vec = as<std::vector<std::string>>(words);
  std::vector<bool> results = cyk_parse(word_vec, inverse_rules, start_symbol);
  return wrap(results);
}
#else
int main() {
  std::unordered_map<Key, std::vector<int>, KeyHash> inverse_rules;
  inverse_rules[std::make_pair(1, 5)] = {6};
  inverse_rules[std::string("b")] = {5};
  inverse_rules[std::string("a")] = {4};
  inverse_rules[std::make_pair(4, 5)] = {3};
  inverse_rules[std::make_pair(5, 4)] = {3};
  inverse_rules[std::make_pair(4, 4)] = {2};
  inverse_rules[std::make_pair(5, 5)] = {2};
  inverse_rules[std::make_pair(2, 3)] = {1};
  inverse_rules[std::make_pair(3, 2)] = {1};
  inverse_rules[std::make_pair(4, 6)] = {0};

  std::vector<std::string> words = {"ababbb", "bababa", "abbbab", "aaabbb", "aaaabbb"};

  int start_symbol = 0;

  std::vector<bool> results = cyk_parse(words, inverse_rules, start_symbol);
  for (size_t i = 0; i < words.size(); ++i) {
    std::cout << "Word: " << words[i] << " is " << (results[i] ? "in" : "not in")
              << " the language." << std::endl;
  }
  return 0;
}
#endif