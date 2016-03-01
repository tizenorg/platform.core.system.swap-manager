#!/bin/bash
ui_viewer_lib="/usr/lib/da_ui_viewer.so"

output=$1


function print_header()
{
    filename=$1
    echo -e "#!/bin/bash\n"\
            "#Preload\n" > $filename
}

function print_ui_viewer_lib()
{
    filename=$1
    echo -e "/bin/echo \"$ui_viewer_lib\" > /sys/kernel/debug/swap/uihv/path" >> $filename
}

##################################
#       Script entry point       #
##################################


print_header $output
print_ui_viewer_lib $output
