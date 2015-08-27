#!/bin/bash


script_dir=$(readlink -f $0 | xargs dirname)
source $script_dir/dyn_vars

if [ "$__tizen_profile_name__" == "tv" ]; then
	webkit_package_name=webkit2-efl-tv
else
	webkit_package_name=webkit2-efl
fi

path_libewebkit2=`rpm -ql $webkit_package_name | grep "libewebkit2.so$" | head -1`

if [ "$__tizen_profile_name__" == "1" ]; then
	path_libewebkit2_debuginfo=$path_libewebkit2
else
	path_libewebkit2_debuginfo=`rpm -ql ${webkit_package_name}-debuginfo | grep "libewebkit2.so.debug$" | head -1`
fi



g_names=()
g_nick_names=()

g_names_plt=()
g_nick_names_plt=()

get_addrs()
{
	param=$1
	shift
	names=($@)
	len=${#names[@]}

	if [[ "$param" == "-r" ]]; then
		path_lib=$path_libewebkit2

		for (( i=0; i < ${len}; ++i)); do
			names[$i]=`echo ${names[$i]} | cut -d'@' -f 1`
		done
	else
		path_lib=$path_libewebkit2_debuginfo
	fi

	addrs=($(parse_elf $path_lib $param ${names[@]}))

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

add_func()
{
	name=$1
	nick_name=$2

	g_names+=($name)
	g_nick_names+=($nick_name)
}

add_func_plt()
{
	name=$1
	nick_name=$2

	g_names_plt+=($name)
	g_nick_names_plt+=($nick_name)
}

gen_array()
{
	addrs_plt=`get_addrs -r ${g_names_plt[@]}`
	if [ $? -ne 0 ]; then
		exit 1;
	fi

	addrs=`get_addrs -sf ${g_names[@]}`
	if [ $? -ne 0 ]; then
		exit 1;
	fi

	addrs=($addrs_plt $addrs)
	g_names=(${g_names_plt[@]} ${g_names[@]})
	g_nick_names=(${g_nick_names_plt[@]} ${g_nick_names[@]})

	len=${#g_names[@]}
	for (( i=0; i < ${len}; ++i)); do
		echo -e "\t{ \"${g_names[$i]}\", \"${g_nick_names[$i]}\", ${addrs[$i]} },"
	done
}


if [ "$__tizen_profile_name__" == "tv" ] || [ "$__tizen_product_2_4_wearable__" == "1" ]; then
	add_func_plt soup_requester_request_uri@plt soup_request
	add_func _ZN7WebCore14ResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE main_res_will
	add_func _ZN7WebCore11CachedImage7addDataEPKcj main_res_add
	add_func _ZN7WebCore14ResourceLoader16didFinishLoadingEd main_res_finish
	add_func _ZN7WebCore14ResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE res_will
	add_func _ZN7WebCore14ResourceLoader16didFinishLoadingEPNS_14ResourceHandleEd res_finish
	add_func _ZN7WebCore22CompositingCoordinator24flushPendingLayerChangesEv redraw
else
	add_func_plt soup_requester_request@plt soup_request
	add_func _ZN7WebCore18MainResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE main_res_will
	add_func _ZN7WebCore18MainResourceLoader7addDataEPKcib main_res_add
	add_func _ZN7WebCore18MainResourceLoader16didFinishLoadingEd main_res_finish
	add_func _ZN7WebCore14ResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE res_will
	add_func _ZN7WebCore14ResourceLoader16didFinishLoadingEPNS_14ResourceHandleEd res_finish
	add_func _ZN6WebKit20LayerTreeCoordinator24flushPendingLayerChangesEv redraw
fi

IFS=$'\n'

array=`gen_array`
if [ $? -ne 0 ]; then
	exit 1
fi



cat << EOF
/*
 * Autogenerated header
 */

#ifndef __WSP_DATA__
#define __WSP_DATA__

struct wsp_data {
	const char *name;
	const char *nick_name;
	unsigned long addr;
};

static struct wsp_data wsp_data[] = {
${array}
};

enum {
	wsp_data_cnt = sizeof(wsp_data) / sizeof(struct wsp_data)
};

#endif /* __WSP_DATA__ */
EOF

