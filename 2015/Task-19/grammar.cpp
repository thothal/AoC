#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using Token = std::string;
using DoubleToken = std::pair<Token, std::optional<Token>>;

class Grammar {
  private:
    std::set<Token> terminals;
    std::set<Token> non_terminals;
    std::set<Token> original_rules;
    std::map<Token, size_t> bin_cnts;
    std::map<Token, std::set<DoubleToken>> rules;
    std::map<DoubleToken, std::set<Token>> inverse_rules;

    Token make_nonterminal(const Token&, bool);
    std::vector<Token> tokenize_input(const Token&) const;
    void add_rule(const Token&, const std::vector<Token>&);

    bool is_original_rule(const Token&, const DoubleToken&) const;

    std::map<Token, unsigned int> run_cyk(const std::vector<Token>&) const;

  public:
    Grammar(const std::map<Token, std::vector<std::vector<Token>>>&);

    bool is_member(const Token&) const;
    bool is_member(const std::vector<Token>&) const;

    unsigned int get_nr_replacements(const Token&) const;
    unsigned int get_nr_replacements(const std::vector<Token>&) const;

    friend std::ostream& operator<<(std::ostream&, const Grammar&);
};

Grammar::Grammar(const std::map<Token, std::vector<std::vector<Token>>>& rules) {
  for (const auto& [symbol, rule] : rules) {
    for (const auto& r : rule) {
      add_rule(symbol, r);
    }
  }
}

bool Grammar::is_member(const std::vector<Token>& word_list) const {
  return run_cyk(word_list).size() > 0;
}

bool Grammar::is_member(const Token& word) const {
  return is_member(tokenize_input(word));
}

unsigned int Grammar::get_nr_replacements(const std::vector<Token>& word_list) const {
  std::map<Token, unsigned int> cyk_results = run_cyk(word_list);
  unsigned int result = -1; // set to maximum value
  for (const auto& [key, n] : cyk_results) {
    result = (n < result) ? n : result;
  }
  return result;
}

unsigned int Grammar::get_nr_replacements(const Token& word) const {
  return get_nr_replacements(tokenize_input(word));
}

std::ostream& operator<<(std::ostream& stream, const Grammar& grammar) {
  stream << "Terminals:" << std::endl << "\t[";
  for (auto it = grammar.terminals.begin(); it != grammar.terminals.end(); ++it) {
    stream << "\"" << *it << "\"";
    if (next(it) != grammar.terminals.end()) {
      stream << ", ";
    }
  }
  stream << "]" << std::endl;
  stream << "Non-Terminals:" << std::endl << "\t[";
  for (auto it = grammar.non_terminals.begin(); it != grammar.non_terminals.end(); ++it) {
    stream << *it;
    if (next(it) != grammar.non_terminals.end()) {
      stream << ", ";
    }
  }
  stream << "]" << std::endl;
  stream << "Original Rules:" << std::endl << "\t[";
  for (auto it = grammar.original_rules.begin(); it != grammar.original_rules.end(); ++it) {
    stream << *it;
    if (next(it) != grammar.original_rules.end()) {
      stream << ", ";
    }
  }
  stream << "]" << std::endl << "Rules:" << std::endl;
  for (const auto& rule : grammar.rules) {
    stream << "\t" << rule.first << " => ";
    auto it = rule.second.begin();
    while (it != rule.second.end()) {
      if (it->second) {
        stream << it->first << " " << *(it->second);
      } else {
        stream << "\"" << it->first << "\"";
      }
      ++it;
      if (it != rule.second.end()) {
        stream << " | ";
      }
    }
    stream << std::endl;
  }
  return stream;
}

Token Grammar::make_nonterminal(const Token& symbol, bool add_suffix = false) {
  Token nt = "NT_" + symbol;
  if (add_suffix) {
    size_t suffix = bin_cnts[symbol] + 1;
    nt = nt + "_" + std::to_string(suffix);
    bin_cnts[symbol]++;
  }
  return nt;
}

std::vector<Token> Grammar::tokenize_input(const Token& word) const {
  Token token;
  std::vector<Token> word_list;
  for (auto it = word.begin(); it != word.end(); ++it) {
    token = *it;
    if (std::next(it) != word.end() && std::islower(*std::next(it))) {
      token += *std::next(it);
      ++it; // Skip the next character as it is already added
    }
    word_list.push_back({token});
  }
  return word_list;
}

void Grammar::add_rule(const Token& lhs, const std::vector<Token>& rhs) {
  DoubleToken p1;
  Token nt_lhs, nt_rhs_left, nt_rhs_right, nt_prev;
  bool need_bin = rhs.size() > 2;
  nt_prev = make_nonterminal(lhs);
  original_rules.insert(nt_prev); // add lhs to original rules needed to count only them
  for (size_t i = 0; i < rhs.size() - 1; ++i) {
    /*
     * Original string: H => CRnFYFYFAr
     * lhs = "H"
     * rhs = {"C", "Rn", "F", "Y", "F", "Ar"}
     * NT_H => NT_C NT_H_1     [0]
     * NT_H_1 => NT_Rn NT_H_2  [1]
     * NT_H_2 => NT_F NT_H_3   [2]
     * NT_H_3 => NT_Y NT_H_4   [3]
     * NT_H_4 => NT_F NT_Ar    [4]
     * NT_H => "H"
     * NT_C => "C"
     * NT_Rn = "Rn"
     * NT_F => "F"
     * NT_Y => "Y",
     * NT_Ar => "Ar"
     */
    terminals.insert(rhs[i]);
    terminals.insert(rhs[i + 1]);
    nt_rhs_left = make_nonterminal(rhs[i]); // NT_C
    p1 = std::make_pair(rhs[i], std::nullopt);
    rules[nt_rhs_left].insert(p1); // NT_C => "C"
    inverse_rules[p1].insert(nt_rhs_left);
    if (need_bin && i != (rhs.size() - 2)) {
      nt_rhs_right = make_nonterminal(lhs, true);
    } else {
      nt_rhs_right = make_nonterminal(rhs[i + 1]);
      p1 = std::make_pair(rhs[i + 1], std::nullopt);
      rules[nt_rhs_right].insert(p1); // NT_Ar => "Ar"
      inverse_rules[p1].insert(nt_rhs_right);
    }
    p1 = std::make_pair(nt_rhs_left, nt_rhs_right);
    rules[nt_prev].insert(p1);
    inverse_rules[p1].insert(nt_prev);
    non_terminals.insert(nt_rhs_left);
    non_terminals.insert(nt_rhs_right);
    non_terminals.insert(nt_prev);
    nt_prev = nt_rhs_right;
  }
}

bool Grammar::is_original_rule(const Token& rule, const DoubleToken& rhs) const {
  return original_rules.find(rule) != original_rules.end() && rhs.second.has_value();
}

std::map<Token, unsigned int> Grammar::run_cyk(const std::vector<Token>& word_list) const {
  DoubleToken p1;
  std::set<Token> rhs;
  size_t n = word_list.size();

  std::map<int, std::map<int, std::map<Token, unsigned int>>> token_table;
  for (size_t j = 0; j < n; ++j) {
    p1 = std::make_pair(word_list[j], std::nullopt);
    if (inverse_rules.find(p1) != inverse_rules.end()) {
      rhs = inverse_rules.at(p1);
      for (const auto& r : rhs) {
        token_table[j][j][r] = 0U;
      }
    }
    for (int i = j; i >= 0; --i) {
      for (size_t k = i; k <= j; ++k) {
        for (const auto& [rl, il] : token_table[i][k]) {
          for (const auto& [rr, ir] : token_table[k + 1][j]) {
            p1 = std::make_pair(rl, rr);
            if (inverse_rules.find(p1) != inverse_rules.end()) {
              rhs = inverse_rules.at(p1);
              for (const auto& r : rhs) {
                token_table[i][j][r] = il + ir + (is_original_rule(r, p1) ? 1U : 0U);
              }
            }
          }
        }
      }
    }
  }
  return token_table[0][n - 1];
}

#ifndef STANDALONE
// [[Rcpp::export]]
unsigned int count_replacements(List rules, Token molecule) {
  Token symbol;
  CharacterVector symbols = rules.names();
  std::map<Token, std::vector<std::vector<Token>>> rule_map;
  for (R_xlen_t i = 0; i < rules.size(); ++i) {
    symbol = as<Token>(symbols[i]);
    std::vector<Token> rule_vec = as<std::vector<Token>>(rules[i]);
    rule_map[symbol].push_back(rule_vec);
  }
  Grammar grammar(rule_map);
  return grammar.get_nr_replacements(molecule);
}
#else
int main() {
  std::map<Token, std::vector<std::vector<Token>>> rules = {{"H", {{"H", "O"}, {"O", "H"}}},
                                                            {"O", {{"H", "H"}}}};
  Grammar grammar(rules);
  Token molecule = "HOH";
  unsigned int replacements = grammar.get_nr_replacements(molecule);
  std::cout << "Number of replacements for molecule '" << molecule << "': " << replacements
            << std::endl;
  return 0;
}
#endif
