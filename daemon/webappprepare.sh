#!/bin/sh

[ $# -eq  0 ] && echo "Usage: $0 <web app path>" && exit 1

app=$1
[ ! -e $app ] && echo "Web app \"$app\" not found" && exit 1

appdir=`echo $app | sed 's:\(.*\)/bin/.*:\1:'`
appid=${appdir##*/}

jscript=/opt/swap/sdk/profile.js

index=`find $appdir -name "index.html" 2>/dev/null`
[ ! -e $index ] && echo "index.html not found" && exit 1

line="<script src=\"js/profile.js\"></script>"
grep "$line" $index 1>&2>/dev/null
if [ $? -ne 0 ]; then
	ln=`cat $index | grep -n '</script>' | tail -1 | cut -f1 -d:`
	ln=`expr $ln \+ 1`

	sed -i "$ln i\ $line" $index
fi

cd $appdir/data

> js_log
> js_trace

#ln -fs $jscript $appdir/res/wgt/js/
cp $jscript $appdir/res/wgt/js/
ln -fs $appdir/data/js_log /opt/swap/sdk/
ln -fs $appdir/data/js_trace /opt/swap/sdk/

chsmack -a $appid $index
#chsmack -a $appid $jscript
chsmack -a $appid $appdir/res/wgt/js/profile.js
chsmack -a $appid js_log
chsmack -a $appid js_trace
