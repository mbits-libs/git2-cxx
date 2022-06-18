@echo off

cd build
cmake .
cmake --build . --config Debug -- -nologo -m -v:n
cmake --build . --target cov_coveralls --config Debug
cd ..
