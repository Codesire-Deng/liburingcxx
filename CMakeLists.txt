cmake_minimum_required(VERSION 3.10.0)
project(iptrail VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-DDEBUG)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(./src)

# include(CTest)
# enable_testing()

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
