#!/bin/bash

get_addrs()
{
	names=($@)
	addrs=($(parse_elf ${lib_file} -sf ${names[@]}))

	len=${#names[@]}
	for (( i=0; i < ${len}; ++i)); do
		name=${names[$i]}
		addr=${addrs[$i]}
		if [ -z "$addr" -o $((16#$addr)) -eq 0 ]; then
			addr=0
			echo "ERROR: symbol '$name' not found" >&2
		fi
		echo 0x$addr
	done
}

script_dir=$(readlink -f $0 | xargs dirname)
source $script_dir/dyn_vars

if [ "$__tizen_profile_name__" == "tv" ]; then
	webkit_package_name=webkit2-efl-tv
else
	webkit_package_name=webkit2-efl
fi

if [ "$__tizen_product_tv__" == "1" ]; then
	lib_file=$(rpm -ql ${webkit_package_name} | grep "/usr/lib/libewebkit2.so$" | head -1)
else
	lib_file=$(rpm -ql ${webkit_package_name}-debuginfo | grep "/usr/lib/debug/usr/lib/libewebkit2.so.debug$" | head -1)
fi

tmp=$(mktemp)

func_names=()

if [ "$__tizen_profile_name__" == "tv" ]; then
	func_names+=(ewk_view_inspector_server_start)
else
	func_names+=(ewk_context_inspector_server_start)
fi
func_names+=(_ZN3JSC16ProfileGenerator11willExecuteEPNS_9ExecStateERKNS_14CallIdentifierE)
func_names+=(_ZN3JSC16ProfileGenerator10didExecuteEPNS_9ExecStateERKNS_14CallIdentifierE)

addrs=(`get_addrs ${func_names[@]}`)

inspector_addr=${addrs[0]}
willexecute_addr=${addrs[1]}
didexecute_addr=${addrs[2]}

rm ${tmp}

if [ -z "${inspector_addr}" -o -z "${willexecute_addr}" -o -z "${didexecute_addr}" ]; then
	exit 1
fi

cat << EOF
/*
 * Autogenerated header
 */

#ifndef WSI_PROF_H_
#define WSI_PROF_H_


static const unsigned long INSPECTOR_ADDR = ${inspector_addr};
static const unsigned long WILLEXECUTE_ADDR = ${willexecute_addr};
static const unsigned long DIDEXECUTE_ADDR = ${didexecute_addr};

#endif /* WSI_PROF_H_ */
EOF
