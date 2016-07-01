#!/bin/bash
handlers_lib="/usr/lib/da_probe_tizen.so"
libc_pkg_name="glibc"
libpthread_pkg_name="glibc"
libsmack_pkg_name="smack"

output=$1


function print_header()
{
    filename=$1
    echo -e "#!/bin/bash\n"\
            "#Preload\n"\
	    "PATH=/bin:/usr/bin:/sbin:/usr/sbin\n" > $filename
}

function print_probe_lib()
{
    filename=$1
    echo -e "/bin/echo \"$handlers_lib\" > /sys/kernel/debug/swap/preload/handler" >> $filename
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
print_probe_lib $output
print_ignored $output

# check addresses
grep 0x00000000 $output && echo "ERROR: generate preload info" >&2 && exit 1
echo 0
