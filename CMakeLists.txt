cmake_minimum_required(VERSION 3.0)

project(ProteusServer)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SRC_LIST ProteusServer/WinSockClasses.cpp ProteusServer/WinSockBase.cpp ProteusServer/Utils.cpp ProteusServer/ProteusServer.cpp ProteusServer/stdafx.cpp)

add_executable(ProteusServer ${SRC_LIST})

add_subdirectory(ProteusClient) 

include(cmake/googletest.cmake)
fetch_googletest(
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_BINARY_DIR}/googletest
    )

enable_testing()
add_subdirectory(UnitTest)