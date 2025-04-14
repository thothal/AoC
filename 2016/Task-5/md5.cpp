#include <Rcpp.h>
#include <openssl/evp.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <regex>

using namespace Rcpp;

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

// [[Rcpp::export]]
std::string get_code(std::string door_id, int len_out, int nr_zeros) {
  std::string code, md5, key, pattern;
  pattern = "^0{" + std::to_string(nr_zeros) + "}";
  std::regex re(pattern);
  int n = 0, i = 0;
  while (n < len_out) {
    key = door_id + std::to_string(i++);
    md5 = get_md5(key);
    if (std::regex_search(md5, re)) {
      code += md5[nr_zeros];
      n++;
    }
  }
  return code;
}

// [[Rcpp::export]]
std::string get_improved_code(std::string door_id, int len_out, int nr_zeros) {
  std::string code(len_out, 'x'), md5, key, pattern;
  pattern = "^0{" + std::to_string(nr_zeros) + "}";
  std::regex re(pattern);
  int n = 0, i = 0, pos;
  while (n < len_out) {
    key = door_id + std::to_string(i++);
    md5 = get_md5(key);
    if (std::regex_search(md5, re)) {
      std::string pos_str(1, md5[nr_zeros]); 
      pos = std::stoi(pos_str, nullptr, 16);
      if (pos < len_out && code[pos] == 'x') {
        code[pos] = md5[nr_zeros + 1];
        n++;
      }
    }
  }
  return code;
}

