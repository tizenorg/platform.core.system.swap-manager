#!/bin/bash
preload_library_pattern="libdl[.-].*"
preload_library_path="/lib/"
preload_open_function="dlopen"
handlers_lib="/usr/lib/da_probe_tizen.so"
linker_path="/lib/"
linker_sym="_r_debug"
test_bin="/bin/su"
libc_pkg_name="glibc"
libpthread_pkg_name="glibc"
libsmack_pkg_name="smack"

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
    addr=$(parse_elf $preload_lib -sa | grep "$preload_open_function\(@\|$\)" | head -1 | cut -f1 -d' ')

    echo -e "/bin/echo \"$preload_lib\" > /sys/kernel/debug/swap/preload/loader/loader_path" >> $filename
    echo -e "/bin/echo 0x$addr > /sys/kernel/debug/swap/preload/loader/loader_offset" >> $filename
}

function print_probe_lib()
{
    filename=$1
    echo -e "/bin/echo \"$handlers_lib\" > /sys/kernel/debug/swap/preload/handlers_path" >> $filename
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

    echo -e "/bin/echo \"$linker\" > /sys/kernel/debug/swap/preload/linker/linker_path" >> $filename
    echo -e "/bin/echo 0x$r_debug_offset > /sys/kernel/debug/swap/preload/linker/r_debug_offset" >> $filename
}

function print_libc()
{
    filename=$1

    libc_path=$(rpm -ql $libc_pkg_name | grep "/lib/libc" | head -1)
    echo -e "/bin/echo \"$libc_path\" > /sys/kernel/debug/swap/preload/ignored_binaries/bins_add" >> $filename
}

function print_libpthread()
{
    filename=$1

    libpthread_path=$(rpm -ql $libpthread_pkg_name | grep "/lib/libpthread" | head -1)
    echo -e "/bin/echo \"$libpthread_path\" > /sys/kernel/debug/swap/preload/ignored_binaries/bins_add" >> $filename
}

function print_libsmack()
{
    filename=$1

    libsmack_path=$(rpm -ql $libsmack_pkg_name | grep "/lib/libsmack" | head -1)
    echo -e "/bin/echo \"$libsmack_path\" > /sys/kernel/debug/swap/preload/ignored_binaries/bins_add" >> $filename
}

function print_ignored()
{
    filename=$1

    print_libc $filename
    print_libpthread $filename
    print_libsmack $filename
}

##################################
#       Script entry point       #
##################################


print_header $output
print_loader $output
print_probe_lib $output
print_linker $output
print_ignored $output

# check addresses
grep 0x00000000 $output && echo "ERROR: generate preload info" >&2 && exit 1
echo 0
