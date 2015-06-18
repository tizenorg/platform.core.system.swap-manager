#!/bin/sh

if [ ! -e /sys/kernel/debug/swap/writer/raw ]; then

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
