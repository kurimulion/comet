# Documentation 
The primary design goal behind using High Level Synthesis (HLS) is that for simulating the functionality on a host machine,
FPGA synthesis of ASIC synthesis the same C / C++ code can be used, with minimal changes. However since the target platforms,
and tools involved are different in nature, these differences have to be accounted for without having to re-write the major
functionality.

We implemented branch prediction on top of the original RISC-V processor. The branch prediction mechanisms have been added to the `doCycle` function in `/src/core.cpp`. The branch predictor is part of `Core` and can be selected by modifying the class of branch predictor in `core.h`.

## Build
Before running  `make` to build the simulator ensure that the variables `AC` and `AP`,which are the location for the
respective header files `ac_int.h` and `ap_int.h` are set. `ac_int.h` is the header file for Catapult HLS, which 
provides support for bit-width accurate datatypes. Alternatively it can also be downloaded
[here](https://www.mentor.com/hls-lp/downloads/ac-datatypes). Likewise, `ac_int.h` is the header file for Vivado HLS.

The `makefile` has been changed to compile vivado-compatible simulator with `make` command.

## Architecture
The 5 stage pipelined architecture currently has 32KB( 8192 * 4 bytes) of instruction memory and 32KB (8192 * 4 bytes) of data memory. 32 bit
Arithmetic and logical integer operations are supported. Currently there is no support for DIV instructions because of the 
complexity of synthesizing division operators. 

Originally, the outcomes of conditional branches were computed at EX stage, which means a taken branch will cause IF and ID stages of the pipeline to be flushed. In order to reduce the overhead and panalty, branch prediction and branch target are computed at ID stage. This is achieved by changing the pipeline registers according to prediction.

## Simulator
In order to execute an elf file compiled by `riscv32-unknown-elf-gcc`, it is necessary to have a process which reads the 
different sections of the binary file, extracts the program instructions and data, and stores it in the instruction and data 
memory. The functions to handle the the elf file are in [elfFile.cpp](src/elfFile.cpp), [elf.h](include/elf.h) and 
[elfFile.h](include/elfFile.h). The process which calls these functions, fills the memory and starts the execution of the 
processor, is defined in [reformeddm_sim.cpp](src/reformeddm_sim.cpp). 

To run simulation:
```
./vivado.sim -f [path-to-executable]
```
content of data memory before execution, cycle counts, and data memory after execution will be printed.

## Compatibility across HLS tools
This is achieved by using macros. Datatypes are defined as `CORE_INT(bits)` or `CORE_UINT(bits)` which at compile time map to 
appropriate definations. The header file [portability.h](include/portability.h) contains definations for these macros.

## Sythesize the design
The design is sythesizable with Vivado HLS. This can be done by running:
```
vivado_hls ./vivado/script.tcl
```