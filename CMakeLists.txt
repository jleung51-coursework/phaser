cmake_minimum_required (VERSION 2.6)
project (BasicServer)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(USER_DIR /home/vagrant)

set(SSL_DIR /usr/lib/x86_64-linux-gnu)
set(Casablanca_DIR ${USER_DIR}/casablanca)
set(Store_DIR ${USER_DIR}/azure-storage-cpp)
set(Test_DIR ${USER_DIR}/cmpt276-projects/unittest-cpp)

find_package(Boost REQUIRED COMPONENTS random chrono system thread regex filesystem)
find_package(Threads REQUIRED)
find_library(CRYPTO crypto ${SSL_DIR})
find_library(SSL    ssl    ${SSL_DIR})

set(REST_LIBRARIES ${Boost_LIBRARIES} ${Boost_FRAMEWORK} ${CRYPTO} ${SSL})

find_library(REST cpprest ${Casablanca_DIR}/Release/build.release/Binaries)

find_library(STORE azurestorage ${Store_DIR}/Microsoft.WindowsAzure.Storage/build.release/Binaries)

#find_package(UnitTest++ REQUIRED)
find_library(TEST UnitTest++ ${Test_DIR}/builds)
include_directories(${Test_DIR})

include_directories(${Casablanca_DIR}/Release/include)
include_directories(${Store_DIR}/Microsoft.WindowsAzure.Storage/includes)

add_executable (basicserver BasicServer.cpp ServerUtils.cpp ServerUtils.h
  TableCache.cpp TableCache.h)
target_link_libraries (basicserver ${REST} ${REST_LIBRARIES} ${STORE})

add_executable (tester testmain.cpp tester.cpp)
target_link_libraries (tester ${REST} ${REST_LIBRARIES} ${STORE} ${TEST})

add_executable (authserver AuthServer.cpp TableCache.cpp TableCache.h)
target_link_libraries (authserver ${REST} ${REST_LIBRARIES} ${STORE})
