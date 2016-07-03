# rtai_depth

make

su root

cd /usr/realtime/modules
insmod rtai_hal.ko
insmod rtai_sched.ko
insmod rtai_shm.ko

cd rtai_depth
./in & ./rtdepth
./out
