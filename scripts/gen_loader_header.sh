#!/bin/bash
loader_library_pattern="libdl[.-].*"
loader_library_path="/lib/"
loader_open_function="dlopen"
linker_path="/lib/"
linker_sym="_r_debug"
test_bin="/bin/su"
output=$1


function print_header()
{
    filename=$1
    echo -e "#!/bin/bash\n"\
            "#Loader\n" > $filename
}

function print_loader()
{
    filename=$1
    el=$(find $loader_library_path -regextype posix-extended -regex $loader_library_path$loader_library_pattern | head -n1)
    loader_lib=$(readlink -f $el)
    addr=$(parse_elf $loader_lib -sa | grep "$loader_open_function\(@\|$\)" | head -1 | cut -f1 -d' ')

    echo -e "/bin/echo \"$loader_lib\" > /sys/kernel/debug/swap/loader/loader/loader_path" >> $filename
    echo -e "/bin/echo 0x$addr > /sys/kernel/debug/swap/loader/loader/loader_offset" >> $filename
}

function print_linker()
{
    filename=$1

    if [ ! -f $test_bin ]; then
        echo "[ERROR] No ${test_bin} to get linker path"
        exit 1
    fi

    el=$(readelf -p .interp $test_bin | grep "/lib/" | awk '{print $3}')
    linker=$(readlink -f $el)
    r_debug_offset=$(readelf -sW $linker | grep $linker_sym | awk '{print $2}' | uniq)

    echo -e "/bin/echo \"$linker\" > /sys/kernel/debug/swap/loader/linker/linker_path" >> $filename
    echo -e "/bin/echo 0x$r_debug_offset > /sys/kernel/debug/swap/loader/linker/r_debug_offset" >> $filename
}

##################################
#       Script entry point       #
##################################


print_header $output
print_loader $output
print_linker $output

# check addresses
grep 0x00000000 $output && echo "ERROR: generate loader info" >&2 && exit 1
echo 0
