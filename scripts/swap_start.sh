#!/bin/sh

terminate_with_error()
{
    echo "$1. error $2"
    exit $2
}

load_module()
{
    module_name="$1"
    if [ ! -f "${module_name}" ];then
        terminate_with_error "file not found <${module_name}>" 123
    fi

    /usr/sbin/insmod ${module_name}
    err=$?
    if [ ! $err -eq 0 ];then
        terminate_with_error "cannot load module <${module_name}>" ${err}
    fi
}

#ERROR CODES
ERR_CONTAINER_NOT_SUPPORTED=1
ERR_NO=0

ERR_CONTAINER_NOT_SUPPORTED_ST="SWAP is not supported devices with container feature"
ERR_NO_ST="No errors"

PATH=$PATH:/usr/sbin/

config_file="/etc/config/model-config.xml"
if [ -e $config_file ]; then
	grep -i "feature/container[^>]*>[[:blank:]]*true" "$config_file" > /dev/null
	if [ $? -eq 0 ]; then
		/bin/echo $ERR_CONTAINER_NOT_SUPPORTED_ST
		exit $ERR_CONTAINER_NOT_SUPPORTED
	fi
fi

if [ ! -e /sys/kernel/debug/swap/enable ]; then

    load_module /opt/swap/sdk/swap_master.ko
    load_module /opt/swap/sdk/swap_buffer.ko
    load_module /opt/swap/sdk/swap_ksyms.ko
    load_module /opt/swap/sdk/swap_driver.ko
    load_module /opt/swap/sdk/swap_writer.ko
    load_module /opt/swap/sdk/swap_kprobe.ko
    load_module /opt/swap/sdk/swap_uprobe.ko
    load_module /opt/swap/sdk/swap_taskctx.ko
    load_module /opt/swap/sdk/swap_us_manager.ko
    load_module /opt/swap/sdk/swap_ks_features.ko
    load_module /opt/swap/sdk/swap_sampler.ko
    load_module /opt/swap/sdk/swap_energy.ko
    load_module /opt/swap/sdk/swap_message_parser.ko
    load_module /opt/swap/sdk/swap_retprobe.ko
    load_module /opt/swap/sdk/swap_fbiprobe.ko
    load_module /opt/swap/sdk/swap_webprobe.ko
    load_module /opt/swap/sdk/swap_preload.ko
    load_module /opt/swap/sdk/swap_wsp.ko
    load_module /opt/swap/sdk/swap_nsp.ko

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

#Preload
if [ -d /sys/kernel/debug/swap/preload/ ]
then
	/usr/bin/swap_init_preload.sh
fi

#WSP
if [ -d /sys/kernel/debug/swap/wsp/ ]
then
	/usr/bin/swap_init_wsp.sh
fi

exit $ERR_NO
