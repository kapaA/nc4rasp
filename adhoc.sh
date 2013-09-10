#!/bin/bash

dev=wlan0
ssid=rlnc-rasp
bssid=4C:12:DC:B6:AF:B7
freq=2462
node=$(cat /etc/hostname)
node=${node:${#node}-2}
ip=10.0.0.1$node/24
bat_ip=10.0.1.1$node/24
bat_mtu=1500
unload=$1
power=100
sock_mem=1048576
packet_loss=1

loaded=$(lsmod | grep -E "^batman_adv")

if [[ ! $loaded = "" && ! "$(batctl if)" = "" ]]; then
    echo Removing $dev from batman-adv
    batctl if del $dev
fi

if [ ! "$loaded" = "" ]; then
    echo Removing batman-adv module
    rmmod batman_adv
fi

if [ ! "$(ip addr show | grep $ip)" = "" ]; then
    echo Deleting ip address
    ip addr del dev $dev $ip
fi

if [ ! $unload = "" ]; then
    exit
fi

if [ ! -d /sys/class/net/$dev ]; then
    echo Interface does not exist: $dev
    exit 1
fi

echo Joining ad-hoc network on $dev
ip link set dev $dev down
iw dev $dev set type ibss
ip link set dev $dev up
iw dev $dev ibss join $ssid $freq $bssid
#iw dev $dev set txpower fixed $power
#iw dev $dev set noack_map 0xffff
#iw dev $dev set bitrates mcs-2.4 0
#iw dev $dev set bitrates legacy-2.4 1 2 5.5
ip addr add dev $dev $ip
ip link set dev $dev mtu 1550
ip link set dev $dev txqueuelen 1000
ip link set dev $dev promisc on

echo Modprobing batman-adv module
modprobe libcrc32c
insmod batman-adv.ko

echo Adding $dev to batman-adv
batctl if add $dev
ip addr add dev bat0 $bat_ip
ip link set dev bat0 mtu $bat_mtu
ip link set dev bat0 up

 set hop penalty for helper
if [ $node -eq 0 ]; then
    echo 200 > /sys/class/net/bat0/mesh/hop_penalty
fi

echo $packet_loss > /sys/class/net/bat0/mesh/packet_loss
echo $sock_mem > /proc/sys/net/core/wmem_max
echo $sock_mem > /proc/sys/net/core/rmem_max
