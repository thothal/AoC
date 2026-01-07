#include <Rcpp.h>

inline bool is_integer64(SEXP x) {
  return TYPEOF(x) == REALSXP && Rf_inherits(x, "integer64");
}

inline void assert_integer64(const Rcpp::NumericVector& v, const char* argname) {
  if (!is_integer64(v)) {
    Rcpp::stop(
      "argument '%s' must be an integer64 vector", argname);
  }
}

inline void read_uint64_buffer(const Rcpp::NumericVector& src, 
                               std::vector<uint64_t>& out) {
  const std::size_t n = src.size();
  out.resize(n);
  if (n > 0) {
    std::memcpy(out.data(), REAL(src), n * sizeof(double));
  }
}

inline Rcpp::NumericVector write_uint64_buffer(const std::vector<uint64_t>& buf) {
  const std::size_t n = buf.size();
  Rcpp::NumericVector res(n);
  if (n > 0) {
    std::memcpy(REAL(res), buf.data(), n * sizeof(double));
  }
  res.attr("class") = Rcpp::CharacterVector::create("integer64");
  return res;
}

template <typename BinOp>
Rcpp::NumericVector bitwOp64(Rcpp::NumericVector a,
                             Rcpp::NumericVector b,
                             BinOp op,
                             const char* opname = "bitwOp64") {
  
  assert_integer64(a, "a");
  assert_integer64(b, "b");
  
  const std::size_t lenA = a.size();
  const std::size_t lenB = b.size();
  
  if (lenA == 0 || lenB == 0) {
    Rcpp::NumericVector res0(0);
    res0.attr("class") = Rcpp::CharacterVector::create("integer64");
    return res0;
  }
  
  const std::size_t n = (lenA > lenB) ? lenA : lenB;
  if ((n % lenA) != 0 || (n % lenB) != 0) {
    Rcpp::warning(
      "longer object length is not a multiple of shorter object length in '%s'", opname);
  }
  
  std::vector<uint64_t> X, Y;
  read_uint64_buffer(a, X);
  read_uint64_buffer(b, Y);
  
  std::vector<uint64_t> res(n);
  
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t ia = i % lenA;
    const std::size_t ib = i % lenB;
    res[i] = op(X[ia], Y[ib]);
  }
  
  return write_uint64_buffer(res);
}

template <typename UnOp>
Rcpp::NumericVector bitwOp64(Rcpp::NumericVector a,
                             UnOp op,
                             const char* opname = "bitwOp64") {
  assert_integer64(a, "a");
  const std::size_t n = a.size();
  std::vector<uint64_t> X;
  read_uint64_buffer(a, X);
  std::vector<uint64_t> res(n);
  for (std::size_t i = 0; i < n; ++i) {
    res[i] = op(X[i]);
  }
  return write_uint64_buffer(res);
}


template <typename ShiftOp>
Rcpp::NumericVector bitwShift64_impl(Rcpp::NumericVector a,
                                     Rcpp::IntegerVector shift,
                                     std::size_t nbits,
                                     ShiftOp op,
                                     const char* opname) {
  assert_integer64(a, "a");
  
  const std::size_t lenA = a.size();
  const std::size_t lenS = shift.size();
  
  if (lenA == 0 || lenS == 0) {
    Rcpp::NumericVector res0(0);
    res0.attr("class") = Rcpp::CharacterVector::create("integer64");
    return res0;
  }
  
  const std::size_t n = (lenA > lenS) ? lenA : lenS;
  if ((n % lenA) != 0 || (n % lenS) != 0) {
    Rcpp::warning("longer object length is not a multiple of shorter object length in '%s'", opname);
  }
  
  const unsigned int width = (nbits >= 64) ? 64u : static_cast<unsigned int>(nbits);
  const uint64_t mask = (width == 0) ? 0ULL
  : (width == 64 ? ~0ULL : ((1ULL << width) - 1ULL));
  
  std::vector<uint64_t> X;
  read_uint64_buffer(a, X);
  
  std::vector<uint64_t> R(n);
  
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t ia = i % lenA;
    const std::size_t is = i % lenS;
    
    int si = shift[is];
    if (si == NA_INTEGER) {
      Rcpp::stop("missing values in 'shift' are not allowed");
    }
    if (si < 0) {
      Rcpp::stop("argument 'shift' must be non-negative");
    }
    const unsigned int k = static_cast<unsigned int>(si);
    
    const uint64_t x = X[ia];
    
    R[i] = op(x, k, mask, width);
  }
  
  return write_uint64_buffer(R);
}


// [[Rcpp::export]]
Rcpp::NumericVector bitwAnd64(Rcpp::NumericVector a, Rcpp::NumericVector b) {
  return bitwOp64(a, b, [](uint64_t x, uint64_t y) { return x & y; }, "bitwAnd64");
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwOr64(Rcpp::NumericVector a, Rcpp::NumericVector b) {
  return bitwOp64(a, b, [](uint64_t x, uint64_t y) { return x | y; }, "bitwOr64");
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwXor64(Rcpp::NumericVector a, Rcpp::NumericVector b) {
  return bitwOp64(a, b, [](uint64_t x, uint64_t y) { return x ^ y; }, "bitwXor64");
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwNot64(Rcpp::NumericVector a, std::size_t nbits = 64) {
  const uint64_t mask = (nbits == 0) ? 0ULL : 
    (nbits >= 64 ? ~0ULL : ((1ULL << nbits) - 1ULL));
  return bitwOp64(a,
                  [mask](uint64_t x) {
                    const uint64_t ux = static_cast<uint64_t>(x);
                    const uint64_t rx = (~ux) & mask;
                    return static_cast<uint64_t>(rx);
                  },
                  "bitwNot64"
  );
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwShiftL64(Rcpp::NumericVector a,
                                 Rcpp::IntegerVector shift,
                                 std::size_t nbits = 64) {
  auto opL = [] (uint64_t x, unsigned int k, uint64_t mask, unsigned int width) -> uint64_t {
    if (width == 0) {
      return 0ULL;  
    }
    const uint64_t v = x & mask;
    if (k >= width) {
      return 0ULL; 
    }
    return (v << k) & mask;
  };
  return bitwShift64_impl(a, shift, nbits, opL, "bitwShiftL64");
}

// [[Rcpp::export]]
Rcpp::NumericVector bitwShiftR64(Rcpp::NumericVector a,
                                 Rcpp::IntegerVector shift,
                                 std::size_t nbits = 64) {
  auto opR = [] (uint64_t x, unsigned int k, uint64_t mask, unsigned int width) -> uint64_t {
    if (width == 0) {
      return 0ULL;  
    }
    const uint64_t v = x & mask;
    if (k >= width) {
      return 0ULL; 
    }
    return (v >> k) & mask;
  };
  return bitwShift64_impl(a, shift, nbits, opR, "bitwShiftR64");
}
