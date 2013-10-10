#!/bin/bash

echo -n "enter raspname"
read -e rasp

wget https://github.com/kapaA/nc4rasp/raw/master/batman-adv.ko
wget raw.github.com/kapaA/nc4rasp/master/adhoc.sh
sudo apt-get install batctl
sudo apt-get install iw
sudo chmod +x adhoc.sh
sudo sed -i '19a\cd /home/pi && sudo ./adhoc.sh' /etc/rc.local 

sudo sed -i '1d' /etc/hostname 
echo $rasp | sudo tee /etc/hostname
wget raw.github.com/kapaA/nc4rasp/master/hosts
sudo rm /etc/hosts
sudo cp hosts /etc/

passwd <<EOF
raspberry
rasp
rasp
EOF

