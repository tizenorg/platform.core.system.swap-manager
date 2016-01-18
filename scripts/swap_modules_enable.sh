#!/bin/sh

/bin/echo 1 > /sys/kernel/debug/swap/enable
/bin/chown -R system:system /sys/kernel/debug/swap
