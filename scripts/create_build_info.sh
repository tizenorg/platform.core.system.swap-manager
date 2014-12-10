#!/bin/sh
DIR=$( cd "$( dirname "$0" )" && pwd );

info_dir="$DIR/../build_info/"
f_git_log="gitlog"
f_git_patch="git.patch"

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

make_local_git()
{
    git --version  || return 0

    add_files=`git status -u | grep "^#" | grep -e "\.h$" -e "\.c$" -e "\.hpp$" -e "\.cpp$" | awk '{print $2}'`

    set -x
    git log > ./${f_git_log}
    git diff > ./${f_git_patch}


    backup_file ./${f_git_log} git
    backup_file ./${f_git_patch} git

    for f in ${add_files};do
        backup_file $f
    done
}

echo "info dir=${info_dir}"
pwd

if [ "$1"="-c" ];then
    rm -r ${info_dir}
    mkdir ${info_dir}
fi

make_local_git

exit 0
