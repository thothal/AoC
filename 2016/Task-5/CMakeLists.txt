cmake_minimum_required(VERSION 3.15)
target_link_libraries(${MODULE_TARGET_NAME} 
  PRIVATE 
  crypto
  crypt32)