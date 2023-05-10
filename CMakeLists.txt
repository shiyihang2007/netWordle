cmake_minimum_required(VERSION 3.3.0)
project(netWordle VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(netWordle main.cpp network.cpp)
if(WIN32)
link_directories("../lib")
target_link_libraries(netWordle "libwsock32.a")
ENDIF()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
