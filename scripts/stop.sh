#!/bin/sh

# swap disabling
echo 0 > /sys/kernel/debug/swap/enable

rmmod swap_wsp
rmmod swap_preload
rmmod swap_task_data
rmmod swap_webprobe
rmmod swap_fbiprobe
rmmod swap_retprobe
rmmod swap_message_parser
rmmod swap_energy
rmmod swap_sampler
rmmod swap_ks_features
rmmod swap_us_manager
rmmod swap_uprobe
rmmod swap_kprobe
rmmod swap_writer
rmmod swap_driver
rmmod swap_ksyms
rmmod swap_buffer
rmmod swap_master
