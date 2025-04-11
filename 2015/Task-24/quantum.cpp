#include <Rcpp.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

using namespace Rcpp;

long long product(const std::vector<int>& nums) {
  long long p = 1;
  for (int n : nums) p *= n;
  return p;
}

bool can_partition(const std::vector<int>& nums, int k, 
                   int target, int start, std::vector<bool>& used) {
  if (k == 1) {
    return true;  
  }
  std::function<bool(int, int)> dfs = [&](int idx, int group_sum) {
    if (group_sum == target) {
      return can_partition(nums, k - 1, target, 0, used);  
    }
    for (size_t i = idx; i < nums.size(); ++i) {
      if (used[i] || group_sum + nums[i] > target) {
        continue;  
      }
      used[i] = true;
      if (dfs(i + 1, group_sum + nums[i])) {
        return true;
      }
      used[i] = false;
    }
    return false;
  };
  
  return dfs(start, 0); 
}

// [[Rcpp::export]]
long long find_min_quantum_entanglement(IntegerVector weights, int k) {
  std::vector<int> nums = as<std::vector<int>>(weights);
  int total = std::accumulate(nums.begin(), nums.end(), 0);
  if (total % k != 0) {
    return -1;
  }
  int target = total / k;
  std::sort(nums.begin(), nums.end(), std::greater<>());
  
  int n = nums.size();
  long long best_qe = LLONG_MAX;
  size_t min_group_size = INT_MAX;
  
  for (int size = 1; size <= n; ++size) {
    std::vector<bool> bitmask(size, true);
    bitmask.resize(n, false);
    
    do {
      std::vector<int> group;
      for (int i = 0; i < n; ++i) {
        if (bitmask[i]) {
          group.push_back(nums[i]);  
        }
      }
      
      if (std::accumulate(group.begin(), group.end(), 0) != target) {
        continue;
      }
      
      std::vector<bool> used(n, false);
      for (int i = 0; i < n; ++i) {
        if (bitmask[i]) {
          used[i] = true; 
        }
      }
      
      if (can_partition(nums, k - 1, target, 0, used)) {
        long long qe = product(group);
        if (group.size() < min_group_size || (group.size() == min_group_size && qe < best_qe)) {
          min_group_size = group.size();
          best_qe = qe;
        }
      }
      
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
    
    if (best_qe != LLONG_MAX) {
      // we found a solution, no need to check groups with more elements
      break; 
    }
  }
  
  return (best_qe == LLONG_MAX) ? -1 : best_qe; 
}
