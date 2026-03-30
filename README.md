# Self-learning Linux Kernel Driver on Beaglebone Green

## 1. Introduction
This repository is dedicated to my self-study journey of Linux kernel driver development, using the Beaglebone Green board and the Debian 13.3 image (`am335x-debian-13.3-base-v6.18-armhf-2026-01-22-4gb.img`).

## 2. Course Overview
### 2.1. Topics Covered and To Be Explored
- Linux kernel and module architecture
- Writing, building, loading, and removing kernel modules
- Character device drivers
- Synchronization, lock-free programming, atomic variables
- Deferred work: jiffies, time delay, workqueue
- User space <-> kernel space communication (file operations, ioctl, sysfs, procfs)
- Kernel module debugging (dmesg, printk, debugfs)
- Optimization techniques and best practices for driver development

*Note: The directory structure and content will be updated and expanded as I progress through the course.*

## 3. Hardware and Environment Setup
### 3.1. Hardware Requirements
- Beaglebone Green board
- SD card (>= 4GB)
- USB cable or suitable power supply

### 3.2. Flashing the OS Image to SD Card
1. Download the image file: `am335x-debian-13.3-base-v6.18-armhf-2026-01-22-4gb.img`
2. Use the following command on Linux to write the image to your SD card (replace `/dev/sdX` with your actual SD card device):
   ```bash
   sudo dd if=am335x-debian-13.3-base-v6.18-armhf-2026-01-22-4gb.img of=/dev/sdX bs=4M status=progress
   sync
   ```
3. Insert the SD card into the Beaglebone Green, power it up, and connect via UART/SSH.

### 3.3. Development Environment Preparation
- Install cross-compile toolchain (if building on PC)
- Install required packages: `build-essential`, `linux-headers`, `make`, etc.
- Connect via SSH or UART to transfer files, build, and load modules

## 4. Building and Loading Kernel Modules
- Navigate to the source code directory
- Run `make` to build
- Load the module:
  ```bash
  sudo insmod <module_name>.ko
  dmesg | tail
  ```
- Remove the module:
  ```bash
  sudo rmmod <module_name>
  ```

## 5. References
- [Linux Device Drivers, 3rd Edition](https://lwn.net/Kernel/LDD3/)
- [Beaglebone Green Wiki](https://wiki.seeedstudio.com/BeagleBone_Green/)
- [Kernel Newbies](https://kernelnewbies.org/)

---
For questions, feedback, or sharing experiences, please contact the repository author.
