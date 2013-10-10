#!/bin/bash

echo -n "enter raspname"
read -e rasp

wget https://github.com/kapaA/nc4rasp/blob/master/batman-adv.ko
wget https://github.com/kapaA/nc4rasp/blob/master/adhoc.sh
sudo apt-get install batctl
sudo apt-get install iw
sudo chmod +x adhoc.sh
sudo sed -i '19a\cd /home/pi && bash sudo adhoc.sh' /etc/rc.local 

sudo sed -i '1d' /etc/hostname 
echo myhost.com | sudo tee /etc/hostname
echo $rasp | sudo tee /etc/hostname
wget https://github.com/kapaA/nc4rasp/blob/master/hosts
sudo rm /etc/hosts
sudo cp hosts /etc/



