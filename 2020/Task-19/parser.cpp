
#include <Rcpp.h>
#include <unordered_set>
#include <vector>
#include <string>
using namespace Rcpp;

// Hilfsfunktion: make_key für 1 oder 2 Elemente
std::string make_key(SEXP b, SEXP c = R_NilValue) {
  if (c == R_NilValue) return as<std::string>(b);
  return as<std::string>(b) + "|" + as<std::string>(c);
}

// Funktion zum Lookup in Liste mit String-Key
IntegerVector list_lookup(List lst, std::string key) {
  CharacterVector names = lst.names();
  for (int i = 0; i < names.size(); ++i) {
    if (key == std::string(names[i])) {
      return lst[i];
    }
  }
  return IntegerVector::create(); // leer, falls nicht gefunden
}

// [[Rcpp::export]]
LogicalVector cyk_rcpp_list(List words, List inv_rules, int start) {
  int n_words = words.size();
  LogicalVector res(n_words);
  
  for (int w = 0; w < n_words; ++w) {
    std::string word = as<std::string>(words[w]);
    int n = word.size();
    if (n == 0) { res[w] = false; continue; }
    
    std::vector< std::vector< std::unordered_set<int> > > V(n, std::vector<std::unordered_set<int>>(n));
    
    // Länge 1 (Terminale)
    for (int i = 0; i < n; ++i) {
      std::string ch(1, word[i]);
      IntegerVector lhs = list_lookup(inv_rules, ch);
      for (int j = 0; j < lhs.size(); ++j) V[i][0].insert(lhs[j]);
    }
    
    // Länge >1
    for (int j = 1; j < n; ++j) {
      for (int i = 0; i + j < n; ++i) {
        std::unordered_set<int> cell;
        for (int k = 0; k < j; ++k) {
          const auto &B = V[i][k];
          const auto &C = V[i+k+1][j-k-1];
          if (B.empty() || C.empty()) continue;
          for (int b : B) {
            for (int c : C) {
              std::string key = std::to_string(b) + "|" + std::to_string(c);
              IntegerVector lhs = list_lookup(inv_rules, key);
              for (int l = 0; l < lhs.size(); ++l) cell.insert(lhs[l]);
            }
          }
        }
        V[i][j] = cell;
      }
    }
    
    res[w] = V[0][n-1].count(start) > 0;
  }
  
  return res;
}

