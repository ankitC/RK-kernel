ONE=1
ZERO=0
echo $ONE > /sys/rtes/config/guarantee
echo $ZERO > /sys/module/cpu_tegra3/parameters/auto_hotplug
#echo $ONE > /sys/devices/system/cpu/cpu0/online
echo $ZERO> /sys/devices/system/cpu/cpu1/online
echo $ZERO> /sys/devices/system/cpu/cpu2/online
echo $ZERO> /sys/devices/system/cpu/cpu3/online
