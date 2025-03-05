#include <Rcpp.h>
#include <array>
#include <set>
#include <string>
using namespace Rcpp;

class NomadSolver {
public:
  NomadSolver(const DataFrame&, bool);
  std::set<std::string> start(bool);
private:
  unsigned int runs;
  bool is_verbose;
  const std::array<int, 9> digits = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::set<std::string> solutions;
  std::array<std::array<int, 3>, 14> params;
  
  void solve(int, int, int, std::string);
};

NomadSolver::NomadSolver(const DataFrame& df, bool verbose = false) :
  runs(0), is_verbose(verbose) {
  int nr_rows = df.nrows(), nr_cols = df.length();
  if (nr_cols != 3 || nr_rows != 14) {
    stop("'df' must have 14 rows and 3 columns (got %i x %i)",
         nr_rows, nr_cols);
  }
  for (R_xlen_t j = 0; j < nr_cols; j++) {
    NumericVector col = df[j];
    for (R_xlen_t i = 0; i < nr_rows; i++) {
      params[i][j] = col[i];
    }
  } 
}

void NomadSolver::solve(int d, int z_goal, int depth, std::string code) {
  int x0, y0, y1, z0, z1, zc;
  const int c1 = params[depth][0], c2 = params[depth][1], c3 = params[depth][2];
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
  for (auto z_new: z_prev) {
    for (auto d_new: digits) {
      solve(d_new, z_new, depth - 1, std::to_string(d_new) + code);
    }
  }  
}

std::set<std::string> NomadSolver::start(bool force = false) {
  if (solutions.empty() || force) {
    solutions.clear();
    runs = 0;
    for (auto d: digits) {
      solve(d, 0, 13, std::to_string(d)); 
    }
    if (is_verbose) {
      Rcout << "Total runs:" << runs << std::endl;
    }
  }
  return solutions;
}


// [[Rcpp::export]]
CharacterVector solve_nomad(const DataFrame& df) {
  NomadSolver solver(df);
  return wrap(solver.start());
}
