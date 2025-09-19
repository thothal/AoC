#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <iomanip>
#include <openssl/evp.h>
#include <regex>
#include <sstream>
#include <string>

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

std::string get_hash(int index,
                     std::string salt,
                     std::vector<std::string>& cache,
                     std::vector<bool>& computed,
                     int stretching_factor) {
  if (index >= (int)cache.size()) {
    cache.resize(index + 1);
    computed.resize(index + 1, false);
  }

  if (computed[index]) {
    return cache[index];
  }

  std::string input = salt + std::to_string(index);
  std::string hash = get_md5(input);
  for (int i = 1; i <= stretching_factor; ++i) {
    hash = get_md5(hash);
  }
  cache[index] = hash;
  computed[index] = true;
  return cache[index];
}

// [[Rcpp::export]]
int find_index(std::string salt, int key_cnt = 64, int stretching_factor = 0) {
  std::vector<std::string> cache;
  std::vector<bool> computed;
  std::vector<int> keys;
  std::string hash;
  std::regex triplet("(.)\\1\\1");
  std::smatch match;
  int i = 0;
  while (keys.size() <= key_cnt) {
    hash = get_hash(++i, salt, cache, computed, stretching_factor);
    if (std::regex_search(hash, match, triplet)) {
      std::string triplet_char = match.str(1);
      std::string quint_pattern = triplet_char + "{5}";
      std::regex quintuple(quint_pattern);
      for (int j = i + 1; j < i + 1001; ++j) {
        hash = get_hash(j, salt, cache, computed, stretching_factor);
        if (std::regex_search(hash, match, quintuple)) {
          keys.push_back(i);
          break;
        }
      }
    }
  }
  return keys[key_cnt - 1];
}

#ifdef STANDALONE
int main() {
  std::cout << find_index("abc", 64, 0) << std::endl;
}
#endif
