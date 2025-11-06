#include <Rcpp.h>

Rcpp::NumericVector bitwOp64(Rcpp::NumericVector a, Rcpp::NumericVector b, 
                             int64_t(*op)(int64_t, int64_t)) {
  size_t len = a.size();
  std::vector<int64_t> res(len), x(len), y(len);
  // integer64 in R are internally stored as double -> use memcpy to copy bit structure
  // reinterpret_cast would work too but results in a compiler warning
  std::memcpy(&(x[0]), &(a[0]), len * sizeof(double));
  std::memcpy(&(y[0]), &(b[0]), len * sizeof(double));
  for (size_t i = 0; i < len; ++i) {
    res[i] = op(x[i], y[i]);
  }
  Rcpp::NumericVector res_r(len);
  std::memcpy(&(res_r[0]), &(res[0]), len * sizeof(double));
  res_r.attr("class") = "integer64";
  return res_r;
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwAnd64(Rcpp::NumericVector a, Rcpp::NumericVector b) {
  return bitwOp64(a, b, [](int64_t x, int64_t y) { return x & y; });
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwOr64(Rcpp::NumericVector a, Rcpp::NumericVector b) {
  return bitwOp64(a, b, [](int64_t x, int64_t y) { return x | y; });
}