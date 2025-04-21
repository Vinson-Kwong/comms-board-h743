## Build
```shell
cmake -B build
cmake --build build -j8
# cmake --build build --target clean
```

## Program with OpenOCD in Linux
```shell
openocd -f openocd.cfg -c "program build/comms-board-h743.elf verify reset exit"
```

## Program in Windows
```shell
openocd -f openocd.cfg -c "program build/Debug/comms-board-h743.elf verify reset exit"
```