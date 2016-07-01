#!/bin/sh

PATH=/bin:/usr/bin:/sbin:/usr/sbin

# swap disabling
/bin/echo 0 > /sys/kernel/debug/swap/enable

/usr/sbin/rmmod swap_uihv
/usr/sbin/rmmod swap_nsp
/usr/sbin/rmmod swap_wsp
/usr/sbin/rmmod swap_preload
/usr/sbin/rmmod swap_loader
/usr/sbin/rmmod swap_webprobe
/usr/sbin/rmmod swap_fbiprobe
/usr/sbin/rmmod swap_retprobe
/usr/sbin/rmmod swap_message_parser
/usr/sbin/rmmod swap_energy
/usr/sbin/rmmod swap_sampler
/usr/sbin/rmmod swap_ks_features
/usr/sbin/rmmod swap_us_manager
/usr/sbin/rmmod swap_taskctx
/usr/sbin/rmmod swap_uprobe
/usr/sbin/rmmod swap_kprobe
/usr/sbin/rmmod swap_writer
/usr/sbin/rmmod swap_driver
/usr/sbin/rmmod swap_ksyms
/usr/sbin/rmmod swap_buffer
/usr/sbin/rmmod swap_master

