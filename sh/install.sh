#/bin/sh

APP=odin_iflytek
if test -z $1 ; then
APP=odin_iflytek
else 
APP=$1
fi

echo "APP=$APP"

echo "OLD"
ps ax | grep -v grep | grep $APP

pkill -9 $APP
sleep 1

spawn-fcgi -a "127.0.0.1" -p 58005 -f /make/BidCore/demo/$APP
sleep 1

echo "NEW"
ps ax | grep -v grep | grep $APP

