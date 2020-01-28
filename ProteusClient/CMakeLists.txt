cmake_minimum_required(VERSION 3.0)

project(ProteusClient)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_LIST ../ProteusServer/WinSockClasses.cpp ../ProteusServer/WinSockBase.cpp ../ProteusServer/Utils.cpp ProteusClient.cpp stdafx.cpp)
add_executable(ProteusClient ${SRC_LIST})

target_include_directories(ProteusClient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ProteusServer)

		  
