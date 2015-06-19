#!/bin/sh

#ERROR CODES
ERR_CONTAINER_NOT_SUPPORTED=1
ERR_NO=0

ERR_CONTAINER_NOT_SUPPORTED_ST="SWAP is not supported devices with container feature"
ERR_NO_ST="No errors"

if [ "$1" != "" ];then
    echo "Error code <$1>. TODO decode it."
    exit $1
fi

PATH=$PATH:/usr/sbin/

config_file="/etc/config/model-config.xml"
if [ -e $config_file ]; then
	grep -i "feature/container[^>]*>[[:blank:]]*false" "$config_file" > /dev/null
	if [ $? -ne 0 ]; then
		echo $ERR_CONTAINER_NOT_SUPPORTED_ST
		exit $ERR_CONTAINER_NOT_SUPPORTED
	fi
fi

if [ ! -e /sys/kernel/debug/swap/enable ]; then

    insmod swap_master.ko           || exit 101
    insmod swap_buffer.ko           || exit 102  # buffer is loaded
    insmod swap_ksyms.ko            || exit 103
    insmod swap_driver.ko           || exit 104  # driver is loaded
    insmod swap_writer.ko           || exit 105
    insmod swap_kprobe.ko           || exit 106  # kprobe is loaded
    insmod swap_uprobe.ko           || exit 107  # uprobe is loaded
    insmod swap_us_manager.ko       || exit 108  # us_manager is loaded
    insmod swap_ks_features.ko      || exit 109  # ks_features is loaded
    insmod swap_sampler.ko          || exit 110
    insmod swap_energy.ko           || exit 111
    insmod swap_message_parser.ko   || exit 112  # parser is loaded
    insmod swap_retprobe.ko         || exit 113  # retprobe is loaded
    insmod swap_fbiprobe.ko         || exit 114  # fbi is loaded
    insmod swap_webprobe.ko         || exit 115  # webprobe is loaded
    insmod swap_task_data.ko        || exit 116
    insmod swap_preload.ko          || exit 117
    insmod swap_wsp.ko              || exit 118
    insmod swap_nsp.ko              || exit 119

fi

# swap enebling
echo 1 > /sys/kernel/debug/swap/enable

# Energy coefficients (before)
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

# cpu idle: 62.27 / 1 (before)
# cpu0 running: 213.21 / 1 (before)
# cpuN running: 97.29 / 1 (before)
# flash read:  74.32 / 33154239 (before)
# flash write : 141.54 / 27920983 (before)

# Energy coefficients (after)
# CPU coefficients
#  - result will expose in uAh
# Flash coefficients
#  - result will expose in uAh

# wifi coefficient
#  - result will expose in uAh

#cpu idle: 3.15234056e-21 * 10^9 * 3600 (after)
echo 11 > /sys/kernel/debug/swap/energy/cpu_idle/numerator &&
echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_idle/denominator &&

# cpu0 running: 9.84873971e-21 * 10^9 * 3600(after)
echo 35 > /sys/kernel/debug/swap/energy/cpu_running/numerator &&
echo 1000000000 > /sys/kernel/debug/swap/energy/cpu_running/denominator &&

# cpuN running: 2.18675205e-20 * 10^9 * 3600(after)
echo 78 > /sys/kernel/debug/swap/energy/cpuN_running/numerator &&
echo 1000000000 > /sys/kernel/debug/swap/energy/cpuN_running/denominator &&

# flash read:   6.52076299e-21 * 10^9 * 3600(after)
echo 23 > /sys/kernel/debug/swap/energy/flash_read/numerator &&
echo 1000000000 > /sys/kernel/debug/swap/energy/flash_read/denominator &&

# flash write : 2.74158641e-19 * 3600(after)
echo 986 > /sys/kernel/debug/swap/energy/flash_write/numerator &&
echo 1000000000 > /sys/kernel/debug/swap/energy/flash_write/denominator &&

# recv : 1.54440608e-15 * 10^9 * 3600
echo 5544 > /sys/kernel/debug/swap/energy/wf_recv/numerator &&
echo 1000000 > /sys/kernel/debug/swap/energy/wf_recv/denominator &&


# send : 1.74609171e-15 * 10^9 * 3600
echo 6264 > /sys/kernel/debug/swap/energy/wf_send/numerator &&
echo 1000000 > /sys/kernel/debug/swap/energy/wf_send/denominator &&

# LCD:
if [ -d /sys/kernel/debug/swap/energy/lcd/ ]
then
	# lcd max (white max - black max) / 2: 255 / 1
	echo 255 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_num` &&
	echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/max_denom` &&

	# lcd min (white min - black min) / 2: 179 / 1
	echo 179 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_num` &&
	echo 1000000 > `ls /sys/kernel/debug/swap/energy/lcd/*/min_denom`
fi

#Preload
if [ -d /sys/kernel/debug/swap/preload/ ]
then
	./init_preload.sh
fi

exit $ERR_NO
