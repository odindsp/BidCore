#/bin/sh

zip -r BidCore.zip ../*
scp BidCore.zip lichunguang@192.168.30.30:/Users/lichunguang/Downloads/
rm -f BidCore.zip

