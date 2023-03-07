
cmake_minimum_required(VERSION 3.22)
project(JsonPacker)
enable_testing()

include(FetchContent)

set(APR_SRC ${CMAKE_CURRENT_BINARY_DIR}/apr-prefix/src/apr)
set(APR_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/apr)

###########
### APR ###
###########

include(cmake/import-apr.cmake)

##############
### Json-C ###
##############

add_subdirectory(json-c)

#############
### Check ###
#############

FetchContent_Declare(
  check
  GIT_REPOSITORY https://github.com/libcheck/check.git
  GIT_TAG        0.15.2
)
FetchContent_MakeAvailable(check)
find_package(Threads REQUIRED)

###################
### Json-Packer ###
###################

set(LIB_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/jp_tlv_encoder.h)

set(LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/jp_tlv_encoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/jp_json_reader.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/jp_key_index_encoder.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/jp_buffer_io.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/jp_invert_key_index_map.c)

add_library(jp_tlv_encoder ${LIB_SOURCES})
#target_link_libraries(jp_tlv_encoder PUBLIC $<$<LINK_LANGUAGE:C>:libapr> $<$<LINK_LANGUAGE:C>:json-c>)
target_link_libraries(jp_tlv_encoder PUBLIC libapr json-c)
target_include_directories(jp_tlv_encoder PUBLIC ${LIB_INCLUDES})

add_executable(example ${CMAKE_CURRENT_SOURCE_DIR}/src/example.c)
target_link_libraries(example PRIVATE $<$<LINK_LANGUAGE:C>:libapr> $<$<LINK_LANGUAGE:C>:json-c>)

add_executable(json_packer ${CMAKE_CURRENT_SOURCE_DIR}/src/json_packer.c)
target_link_libraries(json_packer PRIVATE $<$<LINK_LANGUAGE:C>:libapr> $<$<LINK_LANGUAGE:C>:json-c> $<$<LINK_LANGUAGE:C>:jp_tlv_encoder>)

#########################
### Json-Packer tests ###
#########################

add_executable(json_packer_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/jp_test.c)
target_link_libraries(json_packer_tests PRIVATE jp_tlv_encoder Check::check ${CMAKE_THREAD_LIBS_INIT})