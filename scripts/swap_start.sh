#!/bin/sh

PATH=/bin:/usr/bin:/sbin:/usr/sbin

#ERROR CODES
ERR_CONTAINER_NOT_SUPPORTED=1
ERR_NO=0

ERR_CONTAINER_NOT_SUPPORTED_ST="SWAP is not supported devices with container feature"
ERR_NO_ST="No errors"

if [ "$1" != "" ];then
    /bin/echo "Error code <$1>. TODO decode it."
    exit $1
fi


config_file="/etc/config/model-config.xml"
if [ -e $config_file ]; then
	grep -i "feature/container[^>]*>[[:blank:]]*true" "$config_file" > /dev/null
	if [ $? -eq 0 ]; then
		/bin/echo $ERR_CONTAINER_NOT_SUPPORTED_ST
		exit $ERR_CONTAINER_NOT_SUPPORTED
	fi
fi

if [ ! -e /sys/kernel/debug/swap/enable ]; then

    /usr/sbin/insmod /opt/swap/sdk/swap_master.ko           || exit 101
    /usr/sbin/insmod /opt/swap/sdk/swap_buffer.ko           || exit 102  # buffer is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_ksyms.ko            || exit 103
    /usr/sbin/insmod /opt/swap/sdk/swap_driver.ko           || exit 104  # driver is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_writer.ko           || exit 105
    /usr/sbin/insmod /opt/swap/sdk/swap_kprobe.ko           || exit 106  # kprobe is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_uprobe.ko           || exit 107  # uprobe is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_taskctx.ko          || exit 200
    /usr/sbin/insmod /opt/swap/sdk/swap_us_manager.ko       || exit 108  # us_manager is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_ks_features.ko      || exit 109  # ks_features is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_sampler.ko          || exit 110
    /usr/sbin/insmod /opt/swap/sdk/swap_energy.ko           || exit 111
    /usr/sbin/insmod /opt/swap/sdk/swap_message_parser.ko   || exit 112  # parser is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_retprobe.ko         || exit 113  # retprobe is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_fbiprobe.ko         || exit 114  # fbi is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_webprobe.ko         || exit 115  # webprobe is loaded
    /usr/sbin/insmod /opt/swap/sdk/swap_loader.ko           || exit 116
    /usr/sbin/insmod /opt/swap/sdk/swap_preload.ko          || exit 117
    /usr/sbin/insmod /opt/swap/sdk/swap_wsp.ko              || exit 118
    /usr/sbin/insmod /opt/swap/sdk/swap_nsp.ko              || exit 119
    /usr/sbin/insmod /opt/swap/sdk/swap_uihv.ko             || exit 120

fi

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

# recv through wifi : 1.01901550e-05
/bin/echo 1019 > /sys/kernel/debug/swap/energy/wf_recv/numerator &&
/bin/echo 100000000 > /sys/kernel/debug/swap/energy/wf_recv/denominator &&

# send through wifi : 2.11865993e-05
/bin/echo 2118 > /sys/kernel/debug/swap/energy/wf_send/numerator &&
/bin/echo 100000000 > /sys/kernel/debug/swap/energy/wf_send/denominator &&

# send through bluetooth : 2.53633107e-04
/bin/echo 2536 > /sys/kernel/debug/swap/energy/hci_send_acl/numerator &&
/bin/echo 10000000 > /sys/kernel/debug/swap/energy/hci_send_acl/denominator &&

# recv through bluetooth : 4.08411717e-04
/bin/echo 4084 > /sys/kernel/debug/swap/energy/l2cap_recv_acldata/numerator &&
/bin/echo 10000000 > /sys/kernel/debug/swap/energy/l2cap_recv_acldata/denominator &&

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

#Loader
if [ -d /sys/kernel/debug/swap/loader/ ]
then
	/usr/bin/swap_init_loader.sh
fi

#Preload
if [ -d /sys/kernel/debug/swap/preload/ ]
then
	/usr/bin/swap_init_preload.sh
fi

#UIHV
if [ -d /sys/kernel/debug/swap/uihv/ ]
then
	/usr/bin/swap_init_uihv.sh
fi

#WSP
if [ -d /sys/kernel/debug/swap/wsp/ ]
then
	/usr/bin/swap_init_wsp.sh
fi

exit $ERR_NO
