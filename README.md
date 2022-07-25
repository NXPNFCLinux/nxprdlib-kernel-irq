# NXP NFCRdLib Kernel IRQ Module

## General
This is a linux kernel module for the NXP NFCRdLib IRQ handling of PN5190. 
It allows IRQ handling of PN5190 within the Kernel while the NFCRdLib runs in user space.
This will allow every PN5190 IRQ Raising edge being handled by the NFCRdLib executing in user space.

## Build the Module
### Raspberry Pi
Copy the IRQ Kernel Driver source code from /boot/ to /home/pi and build the Module using below commands on Raspbian Image.
```
cd ~/linux

KERNEL=kernel7
make bcm2709_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
make M=../nxprdlib-kernel-irq/irq
```

## Load and Populate the Module
Module can be loaded using modprobe:
```
sudo insmod /home/pi/nxprdlib-kernel-irq/irq/irq_poll.ko
sudo mknod /dev/irq c 101 0
```
