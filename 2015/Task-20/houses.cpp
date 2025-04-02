#include <Rcpp.h>

int get_sum_of_divisors(unsigned int num, unsigned int limit = -1) {
  int sum = 0;
  for (unsigned int i = 1U; i * i <= num; ++i) {
    if (num % i == 0) {
      unsigned int j = num / i;
      if (j <= limit) {
        sum += i;
      }
      if (i != j && i <= limit) {
        // add reciprocal value unless n = i ^ 2
        sum += j;
      }
    }
  }
  return sum;
}

// [[Rcpp::export]]
unsigned int find_house_number(unsigned int num, unsigned int reward = 10, 
                               unsigned int limit = -1) {
  int i = 1;
  while(reward * get_sum_of_divisors(i, limit) < num) {
    i++;
  }
  return i;
}