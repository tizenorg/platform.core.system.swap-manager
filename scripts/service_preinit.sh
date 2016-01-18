#!/bin/sh

#under root

/bin/mount -o remount,rw / || echo "cannot remount"
/usr/sbin/setcap cap_dac_read_search+ep /usr/bin/da_manager || echo "cannot setcap"

if [ ! -e /tmp/da ];then
    mkdir /tmp/da/
fi

if [ -e /tmp/da/ ];then
    /usr/bin/chsmack -a "*" /tmp/da
    /usr/bin/chmod -R 777 /tmp/da
fi

# TODO remove it
#/bin/chmod -R 755 /usr/bin/launchpad-loader || echo "cannot chmod" >&2

#/bin/echo "swap launchpad-loader rx" | smackload || echo "cannot smackload" >&2
#
#/bin/echo "swap org.example.basicuiapplication rx" | smackload || echo "cannot smackload" >&2
#/bin/echo "org.example.basicuiapplication swap rwx" | smackload || echo "cannot smackload"
#
#/bin/echo "swap com.samsung.clocksetting rx" | smackload || echo "cannot smackload"
#/bin/echo "com.samsung.clocksetting swap rwx" | smackload || echo "cannot smackload"

exit 0
