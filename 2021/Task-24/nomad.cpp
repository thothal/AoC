#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#define Rcout std::cout
#endif
#include <array>
#include <set>
#include <string>

class NomadSolver {
  public:
    NomadSolver(const std::array<std::array<int, 3>, 14>&, bool);
    std::set<std::string> start(bool);

  private:
    unsigned int runs;
    bool is_verbose;
    const std::array<int, 9> digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::set<std::string> solutions;
    std::array<std::array<int, 3>, 14> params;

    void solve(int, int, int, std::string);
};

NomadSolver::NomadSolver(const std::array<std::array<int, 3>, 14>& df, bool verbose = false)
    : runs(0)
    , is_verbose(verbose)
    , params(df) { }

void NomadSolver::solve(int d, int z_goal, int depth, std::string code) {
  int x0, y0, y1, z0, z1, zc;
  std::set<int> z_prev;
  if (++runs % 10000000 == 0) {
    if (is_verbose) {
      Rcout << runs << std::endl;
    }
  }
  if (depth == -1) {
    solutions.insert(code.substr(1));
    return;
  }
  const int c1 = params[depth][0], c2 = params[depth][1], c3 = params[depth][2];
  for (int x1 = 0; x1 <= 1; x1++) {
    y1 = (d + c3) * x1;
    z1 = z_goal - y1;
    y0 = 25 * x1 + 1;
    if ((z1 % y0) == 0) {
      z0 = z1 / y0;
      for (int i = 0; i < c1; i++) {
        zc = z0 * c1 + i;
        x0 = zc % 26 + c2;
        if ((x0 != d) == x1) {
          z_prev.insert(zc);
        }
      }
    }
  }
  for (auto z_new : z_prev) {
    for (auto d_new : digits) {
      solve(d_new, z_new, depth - 1, std::to_string(d_new) + code);
    }
  }
}

std::set<std::string> NomadSolver::start(bool force = false) {
  if (solutions.empty() || force) {
    solutions.clear();
    runs = 0;
    for (auto d : digits) {
      solve(d, 0, 13, std::to_string(d));
    }
    if (is_verbose) {
      Rcout << "Total runs:" << runs << std::endl;
    }
  }
  return solutions;
}
#ifndef STANDALONE
// [[Rcpp::export]]
CharacterVector solve_nomad(const DataFrame& df) {
  int nr_rows = df.nrows(), nr_cols = df.length();
  std::array<std::array<int, 3>, 14> params;
  if (nr_cols != 3 || nr_rows != 14) {
    stop("'df' must have 14 rows and 3 columns (got %i x %i)", nr_rows, nr_cols);
  }
  for (R_xlen_t j = 0; j < nr_cols; j++) {
    NumericVector col = df[j];
    for (R_xlen_t i = 0; i < nr_rows; i++) {
      params[i][j] = col[i];
    }
  }
  NomadSolver solver(params);
  return wrap(solver.start());
}
#else
int main() {
  std::array<std::array<int, 3>, 14> params = {{{1, 10, 1},
                                                {1, 11, 9},
                                                {1, 14, 12},
                                                {1, 13, 6},
                                                {26, -6, 9},
                                                {26, -14, 15},
                                                {1, 14, 7},
                                                {1, 13, 12},
                                                {26, -8, 15},
                                                {26, -15, 3},
                                                {1, 10, 6},
                                                {26, -11, 2},
                                                {26, -13, 10},
                                                {26, -4, 12}}};
  NomadSolver solver(params, true);
  auto solutions = solver.start(true);

  long long max_val = LLONG_MIN;
  for (const auto& str : solutions) {
    long long val = std::stoll(str);
    if (val > max_val) {
      max_val = val;
    }
  }
  std::cout << "Largest solution: " << max_val << std::endl;
  return 0;
}
#endif
