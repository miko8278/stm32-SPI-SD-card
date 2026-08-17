rm -r build/
cmake -DCMAKE_TOOLCHAIN_FILE=stm32f4/toolchain.cmake -B build/ -S .
