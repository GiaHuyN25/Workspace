target remote :3333
symbol-file ./build/${project-name}.elf
mon reset halt
flushregs
thb main