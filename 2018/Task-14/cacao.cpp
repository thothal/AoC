#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif
#include <vector>
#include <algorithm>

// [[Rcpp::export]]
int find_cacao(int n) {
  std::vector<int> target;
  for (int temp = n; temp > 0; temp /= 10) {
    target.push_back(temp % 10);
  }
  std::reverse(target.begin(), target.end());
  
  std::vector<int> quality = {3, 7};
  size_t i = 0, j = 1;
  
  while (true) {
    int sum = quality[i] + quality[j];
    if (sum >= 10) {
      quality.push_back(sum / 10); 
    }
    quality.push_back(sum % 10);
    
    i = (i + 1 + quality[i]) % quality.size();
    j = (j + 1 + quality[j]) % quality.size();
    
    // Check for match at the end
    if (quality.size() >= target.size()) {
      size_t sz = quality.size();
      if (std::equal(target.begin(), target.end(), quality.end() - target.size()))
        return sz - target.size();
      if (sum >= 10 && sz >= target.size() + 1) {
        if (std::equal(target.begin(), target.end(), quality.end() - target.size() - 1))
          return sz - target.size() - 1;
      }
    }
  }
}

#ifdef STANDALONE
int main() {
  std::cout << find_match(704321) << std::endl;
  return 0;
}
#endif
