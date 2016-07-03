#include<csignal>
#include "depth2.0.h"
//#include <time.h>
#include<csignal>
using namespace cv;

static int end = 0;
void endHandler(int sig){
	end = 1;
}

int main(){
    void *shm;
    struct msg_8U *dataout;
    int rptr;
    int i=0;
  //  clock_t start,finish;
    shm = rtai_malloc(SHM1ADDR, sizeof(struct msg_8U));
    dataout = (struct msg_8U *)shm;
    rptr = 0;
    signal(SIGINT,endHandler);
    signal(SIGKILL,endHandler);
    signal(SIGTERM,endHandler);
    signal(SIGALRM,endHandler);
    namedWindow("Disparity");
    //start = clock();
    for(i=0;i<1000&&!end;){        
        Mat Disparity = Mat(Height, Width, CV_8U, dataout->Databuf[rptr]);
        if(dataout->count > 0){
            imshow("Disparity" ,Disparity);
            dataout->count--;
	    rptr = (rptr + 1) % BUFFERSIZE;
	    i++;
        }
	waitKey(10);
    }
   // finish = clock();
   // printf("%ld",finish -start);
    rtai_free(SHM1ADDR, shm);
    return 0;
}
