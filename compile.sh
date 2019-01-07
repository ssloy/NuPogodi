arm-oe-linux-gnueabi-g++ -marm -mcpu=arm926ej-s --sysroot=/opt/trik-sdk/sysroots/arm926ejse-oe-linux-gnueabi/  capture.cpp -c
arm-oe-linux-gnueabi-g++ -marm -mcpu=arm926ej-s --sysroot=/opt/trik-sdk/sysroots/arm926ejse-oe-linux-gnueabi/  yuv2rgb.cpp -c
arm-oe-linux-gnueabi-g++ -marm -mcpu=arm926ej-s --sysroot=/opt/trik-sdk/sysroots/arm926ejse-oe-linux-gnueabi/  dumpframe.cpp -c
arm-oe-linux-gnueabi-g++ -marm -mcpu=arm926ej-s --sysroot=/opt/trik-sdk/sysroots/arm926ejse-oe-linux-gnueabi/  yuv2rgb.o capture.o dumpframe.o 
scp ./a.out root@192.168.1.109:


