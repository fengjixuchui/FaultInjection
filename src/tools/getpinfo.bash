#!/bin/dash
#########################################################################
# File Name: getpinfo.sh
# Author: gatieme
# Created Time: Mon 13 Jun 2016 03:09:31 PM CST
#########################################################################


pid=$1
echo "the pid you can get is ${pid}"

sudo echo ${pid} > /proc/memoryEngine/pid

sudo echo 1 > /proc/memoryEngine/ctl


while true;
do
    ack_signal=`sudo cat /proc/memoryEngine/signal`
    
    if [ ${ack_signal} -eq 1 ];then
        break
    fi;
done;


if [ ${ack_signal} -eq 1 ];then
    sudo cat /proc/memoryEngine/taskInfo
else
    echo "get signal error when get task info [${ack_signal}]"
fi;
