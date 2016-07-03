#include <csignal>
#include "depth2.0.h"
using namespace cv;
static int end = 0;
void endHandler(int sig){
	end = 1;
}
    
int main()
{
    void *shm;
    struct msg_8UC3 *datain;  
    int wptr;
    Mat frame0,frame1;
    //open cameras
    VideoCapture cap0(1), cap1(2);
    if(!cap0.isOpened()){
        printf("Error: could not load camera 0.\n");
        return -1;
    }
    if(!cap1.isOpened()){
        printf("Error: could not load camera 1.\n");
        return -1;
    }
    
    //signal catching
    signal(SIGINT,endHandler);
    signal(SIGKILL,endHandler);
    signal(SIGTERM,endHandler);
    signal(SIGALRM,endHandler);

    //windows for showing images
    namedWindow("frame0");
    namedWindow("frame1");

    //create shared memory
    shm = rtai_malloc(SHM0ADDR, sizeof(struct msg_8UC3));
    datain = (struct msg_8UC3 *)shm;
    datain->count = 0;
    wptr = 0;
    printf("Memory attached.\n");

    while(!end){
        //read images from the cameras
        cap0 >> frame0;
        cap1 >> frame1;
	//printf("%d\n",&frame0);
	//printf("%d\n",frame0.data);
        imshow("frame0", frame0);
        imshow("frame1", frame1);

        memcpy(datain->Databuf[wptr], frame0.data, Height * Width * 3);           //联合体指针???
/*        cvInitMatHeader(datain->Matbuf[wptr],\
                        Height, Width, CV_8UC3,\
                        datain->Databuf[wptr]);*/
        memcpy(datain->Databuf[wptr+1], frame1.data, Height * Width * 3);
/*        cvInitMatHeader(datain->Matbuf[wptr+1],\
                        Height, Width, CV_8UC3,\
                        datain->Databuf[wptr]);*/
        if(datain->count < 4)
		datain->count++;
	else
		datain->count = 4;
 	wptr = (wptr + 2) % BUFFERSIZE;
        waitKey(10);
    }

    rtai_free(SHM0ADDR, shm);

    return 0;
}
