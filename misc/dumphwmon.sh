#!/bin/sh

for ((i=0;i<7;i++))
do
	cat /sys/class/hwmon/hwmon1/device/in${i}_input
done

