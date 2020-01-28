cmake_minimum_required(VERSION 3.0)
project(GTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_LIST ../ProteusServer/Utils.cpp test.cpp pch.cpp)
add_executable(GTest ${SRC_LIST})

target_link_libraries(GTest PUBLIC gtest_main)
target_include_directories(GTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ProteusServer)

add_test(
  NAME
    test
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/GTest
  )
