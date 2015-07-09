#!/bin/bash


gen_define()
{
	name=$1
	val=$2

	echo "#define $name $val"
}

gen_define_str()
{
	name=$1
	val=$2

	echo "#define $name \"$val\""
}

check_null_or_exit()
{
	name=$1
	val=${!1}
	src=$BASH_SOURCE
	line=$BASH_LINENO

	if [ -z "$val" ]; then
		echo "$src:$line ERROR: value '$name' is null" >&2
		exit 1
	fi
}

# get libappcore-efl.so path
path_app_core_efl=$(rpm -ql app-core-efl | grep libappcore-efl | head -1)
check_null_or_exit path_app_core_efl

# get libappcore-efl.so debug_path
dpath_app_core_efl=$(rpm -ql app-core-debuginfo | grep "libappcore-efl\.so\.debug$" | head -1)
check_null_or_exit dpath_app_core_efl

# get launchpad path
path_launchpad=$(rpm -ql launchpad | grep launchpad-loader | head -1)
path_launchpad=${path_launchpad:-$(rpm -ql launchpad | grep launchpad-process-pool | head -1)}
check_null_or_exit path_launchpad


# get appcore_efl_main addr
addr_appcore_efl_main=$(parse_elf ${path_app_core_efl} -s appcore_efl_main)
check_null_or_exit addr_appcore_efl_main

# get __do_app addr
addr_do_app=$(parse_elf ${dpath_app_core_efl} -s __do_app)
check_null_or_exit addr_do_app


tmp=$(mktemp)
# launchpad
addr_dlopen_plt=$(su root -c "parse_elf $path_launchpad -r dlopen")
addr_dlsym_plt=$(su root -c "parse_elf $path_launchpad -r dlsym")

# libappcore-efl.so
addr_appcore_init_plt=$(parse_elf $path_app_core_efl -r appcore_init)
addr_elm_run_plt=$(parse_elf $path_app_core_efl -r elm_run)


# PLT
addr_dlopen_plt=${addr_dlopen_plt:-0}
addr_dlsym_plt=${addr_dlsym_plt:-0}
addr_appcore_init_plt=${addr_appcore_init_plt:-0}
addr_elm_run_plt=${addr_elm_run_plt:-0}

# libappcore-efl
PATH_LIBAPPCORE_EFL=$(gen_define_str PATH_LIBAPPCORE_EFL $path_app_core_efl)
ADDR_APPCORE_EFL_MAIN=$(gen_define ADDR_APPCORE_EFL_MAIN 0x$addr_appcore_efl_main)
ADDR_APPCORE_INIT_PLT=$(gen_define ADDR_APPCORE_INIT_PLT 0x$addr_appcore_init_plt)
ADDR_ELM_RUN_PLT=$(gen_define ADDR_ELM_RUN_PLT 0x$addr_elm_run_plt)
ADDR_DO_APP=$(gen_define ADDR_DO_APP 0x$addr_do_app)

# launchpad
PATH_LAUNCHPAD=$(gen_define_str PATH_LAUNCHPAD $path_launchpad)
ADDR_DLOPEN_PLT_LPAD=$(gen_define ADDR_DLOPEN_PLT_LPAD 0x$addr_dlopen_plt)
ADDR_DLSYM_PLT_LPAD=$(gen_define ADDR_DLSYM_PLT_LPAD 0x$addr_dlsym_plt)


NSP_DEFINES="
/* libappcore-efl */
$PATH_LIBAPPCORE_EFL
$ADDR_APPCORE_EFL_MAIN
$ADDR_DO_APP
$ADDR_APPCORE_INIT_PLT
$ADDR_ELM_RUN_PLT

/* launchpad */
$PATH_LAUNCHPAD
$ADDR_DLOPEN_PLT_LPAD
$ADDR_DLSYM_PLT_LPAD
"

cat << EOF
/*
 * Autogenerated header
 */

#ifndef __NSP_DATA_H__
#define __NSP_DATA_H__
${NSP_DEFINES}
#endif /* __NSP_DATA_H__ */
EOF
