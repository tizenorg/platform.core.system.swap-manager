#!/bin/bash

daemon_dir=`pwd`
lib_file="/usr/lib/debug/usr/lib/libewebkit2.so.debug"
web_inst_file=${daemon_dir}"/daemon/wsi_addr.h"

inspector_addr=`readelf -sW ${lib_file} | grep ewk_context_inspector_server_start | awk '{print "0x"$2}'`
willexecute_addr=`readelf -sW ${lib_file} | grep ProfileGenerator11willExecute | awk '{print "0x"$2}'`
didexecute_addr=`readelf -sW ${lib_file} | grep ProfileGenerator10didExecute | awk '{print "0x"$2}'`

echo "
#ifndef WSI_ADDR_H_
#define WSI_ADDR_H_

enum web_inst_addr {
        INSPECTOR_ADDR = ${inspector_addr},
        WILLEXECUTE_ADDR = ${willexecute_addr},
        DIDEXECUTE_ADDR = ${didexecute_addr}
};

#endif /* WSI_ADDR_H_ */
" > ${web_inst_file}
cat ${web_inst_file}
