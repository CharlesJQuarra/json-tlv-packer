
cmake_minimum_required(VERSION 3.22)
project(Compress2TLV)
enable_testing()

set(APR_SRC ${CMAKE_CURRENT_BINARY_DIR}/apr-prefix/src/apr)
set(APR_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/apr)

include(cmake/import-apr.cmake)
add_subdirectory(json-c)

add_executable(example ${CMAKE_CURRENT_SOURCE_DIR}/src/example.c)
target_link_libraries(example PRIVATE $<$<LINK_LANGUAGE:C>:libapr> $<$<LINK_LANGUAGE:C>:json-c>)

add_executable(json_packer ${CMAKE_CURRENT_SOURCE_DIR}/src/json_packer.c)
target_link_libraries(json_packer PRIVATE $<$<LINK_LANGUAGE:C>:libapr> $<$<LINK_LANGUAGE:C>:json-c>)
