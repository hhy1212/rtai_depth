#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include <rtai_shm.h>
#define SHM0ADDR 0xaaaa
#define SHM1ADDR 0xbbbb
#define BUFFERSIZE 8
#define Height 480
#define Width 640
using namespace cv;

struct msg_8UC3 {
    int count;
//    Mat Matbuf[BUFFERSIZE];
    unsigned char Databuf[BUFFERSIZE][Height * Width * 3];
};

struct msg_8U {
    int count;
//    Mat Matbuf[BUFFERSIZE];
    unsigned char Databuf[BUFFERSIZE][Height * Width];
};
