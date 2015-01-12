#!/bin/bash
preload_library_name="libdl-2.13.so"
preload_library_path="/lib/"
preload_open_function="dlopen@@GLIBC"
handlers_lib="/usr/lib/da_probe_tizen.so"
linker_path="/lib/ld-2.13.so"
linker_sym="_r_debug"

output=$1


function print_header()
{
    filename=$1
    echo -e "# Preload\n"\
            "if [ -d /sys/kernel/debug/swap/preload/ ] \n"\
            "then" >> $filename
}

function print_open_function()
{
    filename=$1
    addr=$2
    echo -e "    echo 0x$addr > /sys/kernel/debug/swap/preload/loader/loader_offset" >> $filename
}

function print_lib_name()
{
    filename=$1
    echo -e "    echo \"$preload_library_path$preload_library_name\" > /sys/kernel/debug/swap/preload/loader/loader_path" >> $filename
}

function print_probe_lib()
{
    filename=$1
    echo -e "    echo \"$handlers_lib\" > /sys/kernel/debug/swap/preload/handlers_path" >> $filename
}

function print_linker_path()
{
    filename=$1
    echo -e "    echo \"$linker_path\" > /sys/kernel/debug/swap/preload/linker/linker_path" >> $filename
}

function print_r_debug_offset()
{
    filename=$1
    offset=$2
    echo -e "    echo 0x$offset > /sys/kernel/debug/swap/preload/linker/r_debug_offset" >> $filename
}

function print_post()
{
    filename=$1
    echo -e "fi" >> $filename
}
##################################
#       Script entry point       #
##################################

open_addr=$(readelf -sW $preload_library_path$preload_library_name | grep $preload_open_function | awk '{print $2}')
r_debug_offset=$(readelf -sW $linker_path | grep $linker_sym | awk '{print $2}' | uniq)

print_header $output
print_lib_name $output
print_open_function $output $open_addr
print_probe_lib $output
print_linker_path $output
print_r_debug_offset $output $r_debug_offset
print_post $output
