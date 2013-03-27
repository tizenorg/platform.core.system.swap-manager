#! /bin/bash

abspath="$(cd "${0%/*}" 2>/dev/null; echo $PWD/${0##*/})"
absdir=`dirname "$abspath"`

PWD=`pwd`

ACTIVATOR=WindowActivator
ACTIVATOR_PATH=${absdir}

# for coredump file
ulimit -c unlimited

${ACTIVATOR_PATH}/${ACTIVATOR} "$@"

