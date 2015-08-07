#!/bin/sh

#ERROR CODES
ERR_CONTAINER_NOT_SUPPORTED=1
ERR_NO=0

ERR_CONTAINER_NOT_SUPPORTED_ST="SWAP is not supported devices with container feature"
ERR_NO_ST="No errors"

if [ "$1" != "" ];then
    /bin/echo "Error code <$1>. TODO decode it."
    exit $1
fi

PATH=$PATH:/usr/sbin/

config_file="/etc/config/model-config.xml"
if [ -e $config_file ]; then
	grep -i "feature/container[^>]*>[[:blank:]]*true" "$config_file" > /dev/null
	if [ $? -eq 0 ]; then
		/bin/echo $ERR_CONTAINER_NOT_SUPPORTED_ST
		exit $ERR_CONTAINER_NOT_SUPPORTED
	fi
fi

#if [ ! -e /sys/kernel/debug/swap/enable ]; then
#
#    /usr/sbin/insmod swap_master.ko           || exit 101
#    /usr/sbin/insmod swap_buffer.ko           || exit 102  # buffer is loaded
#    /usr/sbin/insmod swap_ksyms.ko            || exit 103
#    /usr/sbin/insmod swap_driver.ko           || exit 104  # driver is loaded
#    /usr/sbin/insmod swap_writer.ko           || exit 105
#    /usr/sbin/insmod swap_kprobe.ko           || exit 106  # kprobe is loaded
#    /usr/sbin/insmod swap_uprobe.ko           || exit 107  # uprobe is loaded
#    /usr/sbin/insmod swap_us_manager.ko       || exit 108  # us_manager is loaded
#    /usr/sbin/insmod swap_ks_features.ko      || exit 109  # ks_features is loaded
#    /usr/sbin/insmod swap_sampler.ko          || exit 110
#    /usr/sbin/insmod swap_energy.ko           || exit 111
#    /usr/sbin/insmod swap_message_parser.ko   || exit 112  # parser is loaded
#    /usr/sbin/insmod swap_retprobe.ko         || exit 113  # retprobe is loaded
#    /usr/sbin/insmod swap_fbiprobe.ko         || exit 114  # fbi is loaded
#    /usr/sbin/insmod swap_webprobe.ko         || exit 115  # webprobe is loaded
#    /usr/sbin/insmod swap_task_data.ko        || exit 116
#    /usr/sbin/insmod swap_preload.ko          || exit 117
#    /usr/sbin/insmod swap_wsp.ko              || exit 118
#    /usr/sbin/insmod swap_nsp.ko              || exit 119
#
#fi

# swap enebling
/bin/echo 1 > /sys/kernel/debug/swap/enable

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
/bin/echo 62270 > /sys/kernel/debug/swap/energy/cpu_idle/numerator &&
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_idle/denominator &&

# cpu0 running: 213.21 / 1
/bin/echo 213210 > /sys/kernel/debug/swap/energy/cpu_running/numerator &&
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_running/denominator &&

# cpuN running: 97.29 / 1
/bin/echo 97290 > /sys/kernel/debug/swap/energy/cpuN_running/numerator &&
/bin/echo 1000000000 > /sys/kernel/debug/swap/energy/cpuN_running/denominator &&

# flash read:  74.32 / 33154239
/bin/echo 74320 > /sys/kernel/debug/swap/energy/flash_read/numerator &&
/bin/echo 33154239 > /sys/kernel/debug/swap/energy/flash_read/denominator &&

# flash write: 141.54 / 27920983
/bin/echo 141540 > /sys/kernel/debug/swap/energy/flash_write/numerator &&
/bin/echo 27920983 > /sys/kernel/debug/swap/energy/flash_write/denominator &&

# LCD:
if [ -d /sys/kernel/debug/swap/energy/lcd/ ]
then
	# lcd max (white max - black max) / 2: 255 / 1
	/bin/echo 255 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_num` &&
	/bin/echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_denom` &&

	# lcd min (white min - black min) / 2: 179 / 1
	/bin/echo 179 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_num` &&
	/bin/echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_denom`
fi

#Preload
if [ -d /sys/kernel/debug/swap/preload/ ]
then
	./init_preload.sh
fi

# TODO remove it
#/bin/echo "swap launchpad-loader rx" | smackload
#/bin/echo "swap org.example.basicuiapplication rx" | smackload
#/bin/echo "org.example.basicuiapplication swap rwx" | smackload
#/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.init.service string:no-block
#/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.service string:no-block

exit $ERR_NO
