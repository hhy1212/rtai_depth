# rtai_depth

##This is a realtime stereo vision project,which can calculate depth.
##We used rtai and opencv.


##How to use it
 
make

su root

cd /usr/realtime/modules
insmod rtai_hal.ko
insmod rtai_sched.ko
insmod rtai_shm.ko

cd rtai_depth
./in & ./rtdepth
./out
