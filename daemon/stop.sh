#!/bin/sh

echo 0 > /sys/kernel/debug/swap/enable

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

