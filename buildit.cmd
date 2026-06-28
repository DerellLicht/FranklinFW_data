#  This command only needs to be run *once*, any time the targets change
cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=d:/tdm32/bin/g++.exe
#  this will build the target
cmake --build build

#  this is equivalent to 'make build'
# cmake --build build --target clean

