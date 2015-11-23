#!/bin/sh

set -e

if [ -z "$1" ]; then
  echo "usage: "$0" enable|disable" 1>&2
  exit 1
fi

if [ "$1" = "enable" ]; then
  rm -f /sbin/init
  cat << EOF > /sbin/init
#!/bin/sh

INIT=/usr/lib/systemd/systemd
log=/early_boot.log
swap_dir=/opt/swap/sdk/

if [ "\$$" = 1 ]; then
  /bin/mount -t proc proc proc/
  /bin/mount -t sysfs sys sys/
  /bin/mount -o bind /dev dev/
  /bin/mount -t debugfs none /sys/kernel/debug

  /bin/mount -o remount,rw /
  /bin/mount -t ext4 OPT_PART /opt >> \$log 2>&1

  curr_dir=\$(pwd)
  if [ -d "\$swap_dir" ]; then
     cd \$swap_dir
     ./start.sh >> \$log 2>&1
     cd \$curr_dir
     /usr/bin/swap_readerd >> \$log 2>&1
     sleep 10
  fi
  if [ -f "\$INIT" ]; then
    exec "\$INIT" "\$@"
  fi
  PS1='[recover system] ' exec /bin/sh
fi
EOF
  chmod a+x /sbin/init
elif [ "$1" = "disable" ]; then
  rm -f /sbin/init
  ln -s /usr/lib/systemd/systemd /sbin/init
else
  echo "Unknown option $1" 1>&2
  exit 1
fi
