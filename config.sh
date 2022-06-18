#!/bin/bash

mkdir -p build/conan
cd build/conan
conan install ../.. --build missing -s build_type=Release
conan install ../.. --build missing -s build_type=Debug
cd ..
cmake .. -Dcov_COVERALLS:BOOL=ON -G Ninja
