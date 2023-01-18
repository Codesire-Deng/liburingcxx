cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Setting default CMAKE_BUILD_TYPE to Release")
endif()

project(liburingcxx VERSION 0.7.1 LANGUAGES C CXX)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    message(WARNING "io_uring is only supported by Linux, but the target OS is ${CMAKE_SYSTEM_NAME}.")
endif()

# Get the linux kernel version to liburingcxx
message("target_kernel_version = ${CMAKE_SYSTEM_VERSION}")
set(kernel_version ${CMAKE_SYSTEM_VERSION})
string(REGEX MATCH "^([0-9]+)\.([0-9]+)" kernel_version ${kernel_version})
string(REGEX MATCH "^([0-9]+)" kernel_version_major ${kernel_version})
string(REGEX REPLACE "^([0-9]+)\\." "" kernel_version_patchlevel ${kernel_version})
message("kernel_version_major = ${kernel_version_major}")
message("kernel_version_patchlevel = ${kernel_version_patchlevel}")
add_compile_definitions(LIBURINGCXX_KERNEL_VERSION_MAJOR=${kernel_version_major})
add_compile_definitions(LIBURINGCXX_KERNEL_VERSION_PATCHLEVEL=${kernel_version_patchlevel})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

    if (${CMAKE_BUILD_TYPE} MATCHES Debug)
        message("Enable -fsanitize for gcc")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address,leak -fno-omit-frame-pointer")
    else()
        message(${CMAKE_BUILD_TYPE})
        message("Disable -fsanitize for gcc")
    endif()

    if (${CMAKE_BUILD_TYPE} MATCHES Release)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -flto")
    endif()

    message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
else()
    message(WARNING "Not using GNU g++ for CXX, the compiler id is ${CMAKE_CXX_COMPILER_ID}.")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(./test)

add_subdirectory(./example)
