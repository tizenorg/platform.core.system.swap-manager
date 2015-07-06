#!/bin/bash


script_dir=$(readlink -f $0 | xargs dirname)
source $script_dir/dyn_vars

if [ "$__tizen_profile_name__" == "tv" ]; then
	webkit_package_name=webkit2-efl-tv
else
	webkit_package_name=webkit2-efl
fi

path_libewebkit2=`rpm -ql $webkit_package_name | grep "libewebkit2.so$" | head -1`
path_libewebkit2_debuginfo=`rpm -ql ${webkit_package_name}-debuginfo | grep "libewebkit2.so.debug$" | head -1`


g_names=()
g_nick_names=()


get_addr()
{
	name=$1

	if [[ "$name" == *@plt ]]; then
		addr=$(objdump --section=.plt -d $path_libewebkit2 | grep $name | head -1 | cut -f1 -d' ')
	else
		addr=$(readelf -sW $path_libewebkit2_debuginfo | grep " ${name}\(\.part\.[0-9]*\)*$" | grep " FUNC " | head -1 |awk '{print $2}')
	fi

	if [[ -z "$addr" ]]; then
		echo "ERROR: symbol '$name' not found" >&2
		addr=0
	fi

	echo 0x$addr
}


add_func()
{
	name=$1
	nick_name=$2

	g_names+=($name)
	g_nick_names+=($nick_name)
}


gen_array()
{
	len=${#g_names[@]}
	for (( i=0; i < ${len}; ++i)); do
		name=${g_names[$i]}
		addr=`get_addr $name`
		if [ $? -ne 0 ]; then
			exit 1;
		fi

		echo -e "\t{ \"$name\", \"${g_nick_names[$i]}\", $addr },"
	done
}


if [ "$__tizen_profile_name__" == "tv" ]; then
	add_func soup_requester_request_uri@plt soup_request
	add_func _ZN7WebCore14ResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE main_res_will
	add_func _ZN7WebCore11CachedImage7addDataEPKcj main_res_add
	add_func _ZN7WebCore14ResourceLoader16didFinishLoadingEd main_res_finish
	add_func _ZN7WebCore14ResourceLoader15willSendRequestERNS_15ResourceRequestERKNS_16ResourceResponseE res_will
	add_func _ZN7WebCore14ResourceLoader16didFinishLoadingEPNS_14ResourceHandleEd res_finish
	add_func _ZN7WebCore22CompositingCoordinator24flushPendingLayerChangesEv redraw
else
	add_func soup_requester_request@plt soup_request
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
