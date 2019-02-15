#!/bin/sh

module=dummy
#module=apb_emulator

iverilog-vpi -Istimc -lpcl stimc/${module}.c stimc/stimc.c || exit 1
iverilog -otb_${module}.vvp verilog/${module}.v verilog/tb_${module}.v || exit 1
vvp -M. -m${module} tb_${module}.vvp
