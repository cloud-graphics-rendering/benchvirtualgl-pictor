#ifndef TIME_TRACK
#define TIME_TRACK

#ifndef STOP_BENCH

#include<sys/ipc.h>
#include<sys/shm.h>

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#define TIME_COLUM 10
#define NUM_ROW 100

typedef struct timeTrack{
    int eventID;
    int valid;
    unsigned int array[TIME_COLUM];
}timeTrack;

unsigned int gettime_nanoTime(void)
{
    struct timespec __tv;
    clock_gettime(CLOCK_MONOTONIC,&__tv);
    //clock_gettime(CLOCK_REALTIME,&__tv);
    return(unsigned int)(__tv.tv_sec * 1e9 + __tv.tv_nsec);
}


#endif
#endif
