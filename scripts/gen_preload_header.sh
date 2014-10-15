#!/bin/bash
preload_library_name="libdl-2.13.so"
preload_library_path="/lib/"
preload_open_function="dlopen@@GLIBC"
var_addr_file="/usr/local/include/var_addr"

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

function print_var_offset()
{
    filename=$1
    variabe=$2
    echo -e "    echo 0x$variabe > /sys/kernel/debug/swap/preload/var_offset" >> $filename
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
var_offset=$(cat $var_addr_file)

print_header $output
print_lib_name $output
print_open_function $output $open_addr
print_var_offset $output $var_offset
print_post $output
