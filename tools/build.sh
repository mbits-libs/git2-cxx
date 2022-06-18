#!/bin/bash

cd build
cmake .
ninja
ninja cov_coveralls
