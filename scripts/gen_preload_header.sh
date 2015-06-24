#!/bin/bash
preload_library_pattern="libdl[.-].*"
preload_library_path="/lib/"
preload_open_function="dlopen@@GLIBC"
handlers_lib="/usr/lib/da_probe_tizen.so"
linker_path="/lib/"
linker_sym="_r_debug"
test_bin="/usr/bin/WebProcess"

output=$1


function print_header()
{
    filename=$1
    echo -e "#!/bin/bash\n"\
            "#Preload\n" > $filename
}

function print_loader()
{
    filename=$1
    el=$(find $preload_library_path -regextype posix-extended -regex $preload_library_path$preload_library_pattern | head -n1)
    preload_lib=$(readlink -f $el)
    addr=$(readelf -sW $preload_lib | grep $preload_open_function | head -1 | awk '{print $2}')

    echo -e "echo \"$preload_lib\" > /sys/kernel/debug/swap/preload/loader/loader_path" >> $filename
    echo -e "echo 0x$addr > /sys/kernel/debug/swap/preload/loader/loader_offset" >> $filename
}

function print_probe_lib()
{
    filename=$1
    echo -e "echo \"$handlers_lib\" > /sys/kernel/debug/swap/preload/handlers_path" >> $filename
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

    echo -e "echo \"$linker\" > /sys/kernel/debug/swap/preload/linker/linker_path" >> $filename
    echo -e "echo 0x$r_debug_offset > /sys/kernel/debug/swap/preload/linker/r_debug_offset" >> $filename
}

##################################
#       Script entry point       #
##################################


print_header $output
print_loader $output
print_probe_lib $output
print_linker $output
