#!/bin/sh

#unnecessary
#-/bin/mkdir /tmp/da/

/bin/mkdir /tmp/da/ || echo "warning /tmp/da already exists"
/bin/chmod 777 /tmp/da/ || exit 2


exit 0
