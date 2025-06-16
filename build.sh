git submodule update --init --recursive
cmake -B build/Debug -G Ninja
cmake --build build/Debug -j$(nproc)
