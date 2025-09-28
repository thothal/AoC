#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <algorithm>
#include <array>
#include <unordered_map>
#include <vector>

class LetterContainer {
    std::array<char, 16> data;
    std::unordered_map<char, int> pos;

  public:
    LetterContainer(const std::array<char, 16>& init)
        : data(init) {
      rebuild_map();
    }

    void swap_by_value(char a, char b) {
      int i = pos[a];
      int j = pos[b];
      std::swap(data[i], data[j]);
      pos[a] = j;
      pos[b] = i;
    }

    void swap_by_index(int i, int j) {
      char a = data[i], b = data[j];
      std::swap(data[i], data[j]);
      pos[a] = j;
      pos[b] = i;
    }

    void rotate(int n) {
      std::rotate(data.begin(), data.begin() + (n % 16), data.end());
      rebuild_map();
    }

    void reset() {
      std::sort(data.begin(), data.end());
      rebuild_map();
    }

    std::string str() const { return std::string(data.begin(), data.end()); }

    std::string dance(const std::vector<std::string>& moves, int nr_dances = 1) {
      std::unordered_map<std::string, int> seen;
      std::string state;
      for (int i = 1; i <= nr_dances; i++) {
        state = do_dance(moves);
        if (seen.count(state)) {
          // we have seen this state before
          // now check if it the remainng steps are a multiple of the cycle length
          if ((nr_dances - i) % (i - seen[state]) == 0) {
            break;
          }
        }
        seen[state] = i;
      }
      return state;
    }

  private:
    void rebuild_map() {
      pos.clear();
      for (int i = 0; i < 16; i++)
        pos[data[i]] = i;
    }

    std::string do_dance(const std::vector<std::string>& moves) {
      for (const std::string& move : moves) {
        char type = move[0];
        if (type == 's') {
          int n = std::stoi(move.substr(1));
          rotate(16 - n);
        } else if (type == 'x') {
          size_t slash = move.find('/');
          int a = std::stoi(move.substr(1, slash - 1));
          int b = std::stoi(move.substr(slash + 1));
          swap_by_index(a, b);
        } else if (type == 'p') {
          char a = move[1];
          char b = move[3];
          swap_by_value(a, b);
        }
      }
      return str();
    }
};

#ifndef STANDALONE
// [[Rcpp::export]]
std::string dance(CharacterVector& moves, int nr_dances = 1) {
  LetterContainer lc(std::array<char, 16>(
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'}));
  return lc.dance(as<std::vector<std::string>>(moves), nr_dances);
}

#else
int main() {
  std::vector<std::string> tokens = {"s1", "x3/4", "pe/b"};
  LetterContainer lc(std::array<char, 16>(
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'}));

  std::cout << lc.dance(tokens) << std::endl;
  return 0;
}
#endif
