#!/bin/bash

clang-format -i `find include/ -type f -name *.hpp`
clang-format -i `find src/ example/ test/ -type f -name *.cpp`