#!/bin/sh

export PATH=$PATH:/usr/sbin/

# swap disabling
/usr/bin/dbus-send --system --type=method_call --print-reply --dest=org.tizen.system.busactd /Org/Tizen/System/BusActD org.tizen.system.busactd.SystemD.StartUnit string:swap.disable.service string:wait

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

