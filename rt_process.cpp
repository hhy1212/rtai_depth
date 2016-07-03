#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include "depth2.0.h"
#include <rtai_lxrt.h>
#include <csignal>
//#include <time.h>
using namespace cv;
//the struct of the thread pool
typedef struct {
    pthread_mutex_t queue_lock;            //mutex for work queue
    pthread_cond_t queue_ready;

    int shutdown;
    pthread_t *threadid;
    int max_thread_num;
} CThread_pool;

static CThread_pool *pool = NULL;
//struct pointers for shared memories
struct msg_8UC3 *datain = NULL;
struct msg_8U *dataout = NULL;
static int end = 0;
void endHandler(int sig){
	end = 1;
}

int rptr, wptr;
//int count=0;
void *thread_routine(void *);
//function for initializing the thread pool
void pool_init(int max_thread_num)
{
    pool = (CThread_pool *)malloc(sizeof(CThread_pool));
    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_ready), NULL);
    pool->max_thread_num = max_thread_num;
    pool->shutdown = 0;
    pool->threadid = (pthread_t *)malloc(max_thread_num * sizeof(pthread_t));
    for(int i = 0;i < max_thread_num;i++){
        pthread_create(&(pool->threadid[i]), NULL, thread_routine, NULL);
    }
}
//function for destroy the thread pool
int pool_destroy()
{
    if(pool->shutdown)        //check if the pool has been destroyed
        return -1;

    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->queue_ready));    //broadcast to all the threads 
    for(int i = 0;i < pool->max_thread_num;i++)      //free threads
        pthread_join(pool->threadid[i], NULL);
    free(pool->threadid);
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));

    free(pool);
    pool = NULL;
    return 0;
}
//this part processes the images
int myprocess(Mat frame0, Mat frame1, Mat Disparity8U)
{
    Mat frame0_gray, frame1_gray;
    //--1. Some pretreatments
    flip(frame0, frame0, 1);
    flip(frame1, frame1, 1);
    GaussianBlur(frame0, frame0, Size(5,5), 0, 0);
    GaussianBlur(frame1, frame1, Size(5,5), 0, 0);
    cvtColor(frame0, frame0_gray, CV_BGR2GRAY);
    equalizeHist(frame0_gray, frame0_gray);
    cvtColor(frame1, frame1_gray, CV_BGR2GRAY);
    equalizeHist(frame1_gray, frame1_gray);

    //-- 2. Call the constructor for StereoBM
    int ndisparities = 16*5;   /**< Range of disparity */
    int SADWindowSize = 21; /**< Size of the block window. Must be odd */
    Mat Disparity16S = Mat(Height, Width, CV_16S);

    Ptr<StereoBM> sbm = StereoBM::create( ndisparities, SADWindowSize );
   
    //-- 3. Calculate the disparity image
    sbm->compute( frame0_gray, frame1_gray, Disparity16S );
    //-- Check its extreme values
    double minVal; double maxVal;
    minMaxLoc( Disparity16S, &minVal, &maxVal );
   // printf("Min disp: %f Max value: %f \n", minVal, maxVal);

    //-- 4. Display it as a CV_8UC1 image
    Disparity16S.convertTo(Disparity8U, CV_8UC1, 255/(maxVal - minVal));
 
    return 0;
}
//this is the work that all threads do
void *thread_routine(void * arg)
{
    Mat Disparity8U = Mat(Height, Width, CV_8U);

    printf("Starting thread 0x%lx\n.", pthread_self());
    while(1){
        pthread_mutex_lock(&(pool->queue_lock));
        while(datain->count == 0 && !pool->shutdown){  //while there is no work and the pool still exists, wait
            printf("Thread 0x%lx is waiting.\n", pthread_self());
            sleep(1);
	  // rt_sleep(nano2count(1*1E7));
        }
        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->queue_lock));
            printf("Thread 0x%lx will exit.\n", pthread_self());
            pthread_exit(NULL);
        }
        datain->count--;
        pthread_mutex_unlock(&(pool->queue_lock));

        Mat frame0 = Mat(Height, Width, CV_8UC3, datain->Databuf[rptr]);
        Mat frame1 = Mat(Height, Width, CV_8UC3, datain->Databuf[(rptr + 1) % BUFFERSIZE]);
        myprocess(frame0, frame1, Disparity8U);
        memcpy(dataout->Databuf[wptr], Disparity8U.data, Height * Width);
/*        cvInitMatHeader(dataout->Matbuf[wptr],\
                        Height, Width, CV_8U,\
                        dataout->Databuf[wptr]);*/
        dataout->count++;
        rptr = (rptr + 2) % BUFFERSIZE;
        wptr = (wptr + 1) % BUFFERSIZE;
	//count++;
    }
    pthread_exit(NULL);
}


int main()
{
    void *shm0 = rtai_malloc(SHM0ADDR, sizeof(msg_8UC3));
    void *shm1 = rtai_malloc(SHM1ADDR, sizeof(msg_8U));
  //  clock_t start,finish;
    datain = (struct msg_8UC3 *)shm0;
    dataout = (struct msg_8U *)shm1;

/*    for(int i=0;i<BUFFERSIZE;i++){
        ::new(&(datain->buffer[i]))Mat();
        ::new(&(dataout->buffer[i]))Mat();
    }*/
    rptr = wptr = 0;
    signal(SIGINT,endHandler);
    signal(SIGKILL,endHandler);
    signal(SIGTERM,endHandler);
    signal(SIGALRM,endHandler);
    //create realtime task
    RT_TASK *task;
    static RTIME period, expected;
    if(!(task = rt_task_init_schmod(nam2num("test"), 0, 0, 0, SCHED_FIFO, 0xf))){
        printf("Fail to init\n");
        exit(EXIT_FAILURE);
    }
    mlockall(MCL_CURRENT | MCL_FUTURE);
    period = start_rt_timer(nano2count(1000000));
    expected = rt_get_time() + period * 10;
    rt_task_make_periodic(task, expected, period * 100);

    pool_init(8);
    //start = clock();
    while(!end){
//	printf("%d",count);
//	if(count == 999){
		//finish = clock();
	//	printf("%d",finish - start);
	//	break;
//	}
        rt_task_wait_period();
    }

    pool_destroy();
    rtai_free(SHM0ADDR, shm0);
    rtai_free(SHM1ADDR, shm1);
    stop_rt_timer();
    rt_task_delete(task);

    return 0;
}
