#!/bin/sh

#module=dummy
module=apb_emulator
#mode=c
mode=cpp

iverilog-vpi -Istimc -lpcl stimc/${module}.${mode} stimc/stimc.c stimc/stimc++.cpp || exit 1
iverilog -otb_${module}.vvp verilog/${module}.v verilog/tb_${module}.v || exit 1
vvp -M. -m${module} tb_${module}.vvp
