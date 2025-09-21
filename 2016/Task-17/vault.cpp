#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <iomanip>
#include <openssl/evp.h>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

std::string get_md5(const std::string& input) {
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  const EVP_MD* md = EVP_md5();
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, input.c_str(), input.size());
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);

  std::ostringstream oss;
  for (size_t i = 0; i < md_len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_value[i]);
  }

  EVP_MD_CTX_free(mdctx);
  return oss.str();
}

constexpr int ROOMS_ROWS = 4;
constexpr int ROOMS_COLS = 4;

class Maze {
  private:
    int row;
    int col;
    std::string passkey;
    std::vector<char> path;

    std::string get_current_passkey() const;
    bool is_wall(char dir) const;

  public:
    Maze(const std::string);

    bool is_done() const { return row == ROOMS_ROWS - 1 && col == ROOMS_COLS - 1; };
    int score() const { return ROOMS_ROWS - 1 - row + ROOMS_COLS - 1 - col + path.size(); };
    std::string get_path() const;

    Maze move(char dir) const;

    std::vector<char> get_valid_neighbors() const;
};

struct MazeComparator {
  public:
    bool operator()(const Maze& a, const Maze& b) const {
      return a.score() > b.score(); // lower score = better
    }
};

Maze::Maze(const std::string key)
    : row(0)
    , col(0)
    , passkey(key) { }

std::string Maze::get_path() const {
  return std::string(path.data(), path.size());
}

Maze Maze::move(char dir) const {
  static const std::unordered_map<char, std::pair<int, int>> DELTAS = {
      {'U', {-1, 0}}, {'R', {0, 1}}, {'D', {1, 0}}, {'L', {0, -1}}};

  Maze next_room = *this;
  auto delta = DELTAS.at(dir);
  next_room.row += delta.first;
  next_room.col += delta.second;
  next_room.path.push_back(dir);
  return next_room;
}

std::vector<char> Maze::get_valid_neighbors() const {
  if (is_done()) {
    return {};
  }
  std::vector<char> dirs = {'U', 'D', 'L', 'R'};
  std::vector<char> valid_dirs;
  std::string code = get_md5(get_current_passkey());
  for (int i = 0; i < dirs.size(); ++i) {
    char c = code[i];
    if ((c >= 'b' && c <= 'f') && !is_wall(dirs[i])) {
      valid_dirs.push_back(dirs[i]);
    }
  }
  return valid_dirs;
}

bool Maze::is_wall(char dir) const {
  bool res;
  if (dir == 'U') {
    res = row == 0;
  } else if (dir == 'R') {
    res = col == ROOMS_COLS - 1;
  } else if (dir == 'D') {
    res = row == ROOMS_ROWS - 1;
  } else if (dir == 'L') {
    res = col == 0;
  }
  return res;
}

std::string Maze::get_current_passkey() const {
  return passkey + get_path();
}

// [[Rcpp::export]]
std::string get_shortest_path(std::string passcode) {
  std::string shortest_path;
  std::priority_queue<Maze, std::vector<Maze>, MazeComparator> walk;
  Maze start(passcode);
  walk.push(start);
  while (!walk.empty()) {
    Maze pos = walk.top();
    walk.pop();
    if (pos.is_done()) {
      shortest_path = pos.get_path();
      break;
    }
    std::vector<char> nbs = pos.get_valid_neighbors();
    for (char dir : nbs) {
      walk.push(pos.move(dir));
    }
  }
  return shortest_path;
}

// [[Rcpp::export]]
int get_longest_path(std::string passcode) {
  int longest_path = 0;
  std::queue<Maze> walk;
  Maze start(passcode);
  walk.push(start);

  while (!walk.empty()) {
    Maze pos = walk.front();
    walk.pop();
    if (pos.is_done()) {
      if (pos.get_path().size() > longest_path) {
        longest_path = pos.get_path().size();
      }
    }
    std::vector<char> nbs = pos.get_valid_neighbors();
    for (char dir : nbs) {
      walk.push(pos.move(dir));
    }
  }
  return longest_path;
}

#ifdef STANDALONE
int main() {
  std::cout << get_shortest_path("ihgpwlah") << std::endl
            << get_shortest_path("kglvqrro") << std::endl
            << get_shortest_path("ulqzkmiv") << std::endl;
  std::cout << get_longest_path("ihgpwlah") << std::endl
            << get_longest_path("kglvqrro") << std::endl
            << get_longest_path("ulqzkmiv") << std::endl;
}
#endif
