#!/bin/sh

/bin/echo 0 > /sys/kernel/debug/swap/enable
/bin/chown -R root:root /sys/kernel/debug/swap
