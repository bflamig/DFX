#!/bin/bash
sudo apt-get install jackd2
sudo apt-get install build-essential qtdeclarative5-dev qt5-default qttools5-dev-tools libjack-jackd2-dev
cd ~
rm -r -f jamulus
git clone -b r3_6_0 https://github.com/corrados/jamulus.git
cd jamulus
qmake Jamulus.pro
make clean
sudo make install
sudo adduser -system --no-create-home jamulus
sudo adduser jamulus audio
cd ~
echo "/usr/bin/jackd -T -P95 -p16 -t2000 -d alsa -dhw:U192k -p 64 -n 2 -r 48000" > .jackdrc