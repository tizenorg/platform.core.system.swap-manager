#!/bin/sh
SC_PATH="`dirname \"$0\"`"              # relative
SC_PATH="`( cd \"$SC_PATH\" && pwd )`"  # absolutized and normalized
echo $SC_PATH

build_info="build_info"
info_dir="$SC_PATH/../$build_info/"
f_git_log="gitlog"
f_git_patch="git.patch"

rm -rf ${info_dir}

backup_file()
{
    f=$1
    add_path=$2
    to=${info_dir}/$add_path/$f
    dir=`dirname $to`
    if [ ! -d $dir ];then
        mkdir -p $dir
    fi
    cp -r $f $to

}

mkdir ${info_dir}

set -x
git log > ./${f_git_log}
git diff > ./${f_git_patch}


backup_file ./${f_git_log} git
backup_file ./${f_git_patch} git


for f in `ls $info_dir/..`;do
    if [ "${f##*\/}" = "$build_info" ];then
        echo skip $f
        continue
    fi
    #backup_file $f
    echo "------ $f"
    if [ -d "$f" ];then
      cp -r "$f" "$info_dir"
    else
      cp "$f" "$info_dir"
    fi
done

ls -la $info_dir
