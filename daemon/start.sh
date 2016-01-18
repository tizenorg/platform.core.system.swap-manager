#!/bin/sh

export PATH=$PATH:/usr/sbin/

# swap enable and change access permissions
/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.enable.service string:wait

err=$?
if [ ! $err -eq 0 ];then
    echo "systemd launch error $err"
    exit 2
fi

if [ ! -e /sys/kernel/debug/swap/enable ]; then

    insmod swap_master.ko || exit 1
    insmod swap_buffer.ko || exit 1  # buffer is loaded
    insmod swap_ksyms.ko || exit 1
    insmod swap_driver.ko || exit 1  # driver is loaded
    insmod swap_writer.ko || exit 1
    insmod swap_kprobe.ko || exit 1  # kprobe is loaded
    insmod swap_uprobe.ko || exit 1  # uprobe is loaded
    insmod swap_us_manager.ko || exit 1  # us_manager is loaded
    insmod swap_ks_features.ko || exit 1  # ks_features is loaded
    insmod swap_sampler.ko || exit 1
    insmod swap_energy.ko || exit 1
    insmod swap_message_parser.ko || exit 1  # parser is loaded

fi

# swap enebling
#/bin/echo 1 > /sys/kernel/debug/swap/enable

# Energy coefficients
# CPU coefficients are divided by 10^6 because
#  - they were calculated for mAs
#  - SWAP modules count nanoseconds
#  - result should be exposed in uAs
# Flash coefficients are multiplied by 10^3 because
#  - they were calculated for mAs
#  - result should be exposed in uAs
# LCD coefficients are divided by 10^6 because
#  - they were calculated for mAs
#  - result should be exposed in uAs

# cpu idle: 62.27 / 1
/bin/echo 62270 > /sys/kernel/debug/swap/energy/cpu_idle/numerator
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_idle/denominator

# cpu0 running: 213.21 / 1
/bin/echo 213210 > /sys/kernel/debug/swap/energy/cpu_running/numerator
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_running/denominator

# cpuN running: 97.29 / 1
/bin/echo 97290 > /sys/kernel/debug/swap/energy/cpuN_running/numerator
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpuN_running/denominator

# flash read:  74.32 / 33154239
/bin/echo 74320 > /sys/kernel/debug/swap/energy/flash_read/numerator
/bin/echo 33154239 > /sys/kernel/debug/swap/energy/flash_read/denominator

# flash write: 141.54 / 27920983
/bin/echo 141540 > /sys/kernel/debug/swap/energy/flash_write/numerator
/bin/echo 27920983 > /sys/kernel/debug/swap/energy/flash_write/denominator

# recv through wifi : 1.01901550e-05
/bin/echo 1019 > /sys/kernel/debug/swap/energy/wf_recv/numerator
/bin/echo 100000000 > /sys/kernel/debug/swap/energy/wf_recv/denominator

# send through wifi : 2.11865993e-05
/bin/echo 2118 > /sys/kernel/debug/swap/energy/wf_send/numerator
/bin/echo 100000000 > /sys/kernel/debug/swap/energy/wf_send/denominator

# send through bluetooth : 2.53633107e-04
/bin/echo 2536 > /sys/kernel/debug/swap/energy/hci_send_acl/numerator
/bin/echo 10000000 > /sys/kernel/debug/swap/energy/hci_send_acl/denominator

# recv through bluetooth : 4.08411717e-04
/bin/echo 4084 > /sys/kernel/debug/swap/energy/l2cap_recv_acldata/numerator
/bin/echo 10000000 > /sys/kernel/debug/swap/energy/l2cap_recv_acldata/denominator

# LCD:
if [ -d /sys/kernel/debug/swap/energy/lcd/ ]
then
	# lcd max (white max - black max) / 2: 255 / 1
	/bin/echo 255 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_num`
	/bin/echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_denom`

	# lcd min (white min - black min) / 2: 179 / 1
	/bin/echo 179 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_num`
	/bin/echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_denom`
fi

exit 0

# TODO remove it
#/bin/echo "swap launchpad-loader rx" | smackload
#/bin/echo "swap org.example.basicuiapplication rx" | smackload
#/bin/echo "org.example.basicuiapplication swap rwx" | smackload
#/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.init.service string:no-block
#/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.service string:no-block

