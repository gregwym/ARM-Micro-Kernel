# ARM Micro Kernel

CS 452 "Realtime System" Course Project  
Member: Greg Wang, Tony Zhao

## Overview

This is a micro kernel run on TS-7200 (ARM-based device), following the spec from [http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/pdf/kernel.pdf]()

## How to…

To compile the source, 

* execute the `make clean; make all` command. 
* *make* will compile the program into `./main.elf`

To execute, 

* Load the program from `ARM/y386wang/main.elf`
* Send `go` to start execution

