#include <Rcpp.h>
#include <unordered_set>
#include <unordered_map>

using namespace Rcpp;
using ull = unsigned long long;

inline ull key(int x, int y) {
  return ((ull) (uint32_t) x << 32 ) | (uint32_t) y;
}

inline std::pair<int, int> dekey(ull v) {
  int x = static_cast<int>(static_cast<uint32_t>(v >> 32));
  int y = static_cast<int>(static_cast<uint32_t>(v & 0xFFFFFFFFull));
  return {x, y};
}

// [[Rcpp::export]]
CharacterVector get_proposals(IntegerMatrix elves, List look_to) {
  int ne = elves.nrow();
  CharacterVector res(ne, NA_STRING);
  
  std::unordered_set<ull> elf_set;
  elf_set.reserve(ne * 2);
  
  for (int i = 0; i < ne; ++i) {
    elf_set.insert(key(elves(i, 0), elves(i, 1)));
  }
  
  int n_dirs = look_to.size();
  CharacterVector dir_names = look_to.names();
  
  std::unordered_set<ull> all_nb;
  for (int d = 0; d < n_dirs; ++d) {
    IntegerMatrix m = look_to[d];
    for (int i = 0; i < m.nrow(); ++i) {
      all_nb.insert(key(m(i,0), m(i,1)));
    }
  }
  
  for (int e = 0; e < ne; ++e) {
    int ex = elves(e, 0);
    int ey = elves(e, 1);
    ull self_key = key(ex, ey);
    
    elf_set.erase(self_key);
    
    bool has_nb = false;
    for (ull nb : all_nb) {
      std::pair<int, int> p = dekey(nb);
      if (elf_set.count(key(ex + p.first, ey + p.second))) {
        has_nb = true;
        break;
      }
    }
    
    if (has_nb) {
      for (int d = 0; d < n_dirs; ++d) {
        IntegerMatrix m = look_to[d];
        bool ok = true;
        for (int i = 0; i < m.nrow(); ++i) {
          if (elf_set.count(key(ex + m(i, 0), ey + m(i, 1)))) {
            ok = false;
            break;
          }
        }
        if (ok) {
          res[e] = dir_names[d];
          break;
        }
      }
    }
    
    elf_set.insert(self_key);
  }
  
  return res;
}