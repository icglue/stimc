#!/bin/sh

iverilog-vpi -Isocc -lpcl socc/*.c || exit 1
iverilog -otb_dummy.vvp verilog/*.v || exit 1
vvp -M. -mdummy tb_dummy.vvp
