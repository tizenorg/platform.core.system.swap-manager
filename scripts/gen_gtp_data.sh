#!/bin/bash
linker_path="/lib/"
test_bin="/usr/bin/WebProcess"
handlers_lib="/usr/lib/da_probe_tizen.so"
fixup_handler="__dl_fixup_wrapper"
linker_info_header="/usr/local/include/linker_info.h"

output=$1



function print_header()
{
    filename=$1
    echo -e "#!/bin/bash\n"\
            "#GOT patcher\n" > $filename
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

    echo -e "/bin/echo \"$linker\" > /sys/kernel/debug/swap/got_patcher/linker/path" >> $filename
}

function print_dl_fixup_off()
{
    filename=$1

    addr=$(cat $linker_info_header | grep -o "0x[0-9a-f]\{8\}")
    echo -e "/bin/echo \"$addr\" > /sys/kernel/debug/swap/got_patcher/linker/dl_fixup_addr" >> $filename
}

function print_probe_lib()
{
    filename=$1

    echo -e "/bin/echo \"$handlers_lib\" > /sys/kernel/debug/swap/got_patcher/handler/path" >> $filename
}

function print_dl_fixup_handler_off()
{
    filename=$1

    addr=$(readelf -sW $handlers_lib | grep $fixup_handler | awk '{print \"0x\" $2}')
    echo -e "/bin/echo \"$addr\" > /sys/kernel/debug/swap/got_patcher/handler/fixup_handler_off" >> $filename
}

print_header $output
print_linker $output
print_dl_fixup_off $output
print_probe_lib $output
print_dl_fixup_handler_off $output
