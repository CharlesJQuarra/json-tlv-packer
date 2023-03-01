
cmake_minimum_required(VERSION 3.22)
project(Compress2TLV)
enable_testing()

set(APR_SRC ${CMAKE_CURRENT_BINARY_DIR}/apr-prefix/src/apr)
set(APR_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/apr)
#set(MODBUS_STATIC_LIB ${MODBUS_BIN}/lib/libmodbus.a)

include(cmake/import-apr.cmake)


add_executable(example ${CMAKE_CURRENT_SOURCE_DIR}/src/example.c)
target_link_libraries(example PRIVATE $<$<LINK_LANGUAGE:C>:libapr>)
