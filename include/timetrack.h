#ifndef TIME_TRACK
#define TIME_TRACK

#ifndef STOP_BENCH

#include<sys/ipc.h>
#include<sys/shm.h>

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#define TIME_COLUM 10
#define NUM_ROW 10

typedef struct timeTrack{
    int eventID;
    pid_t pid;
    long array[TIME_COLUM];
}timeTrack;

unsigned long gettime_nanoTime(void)
{
    struct timespec __tv;
    clock_gettime(CLOCK_MONOTONIC,&__tv);
    //clock_gettime(CLOCK_REALTIME,&__tv);
    return(__tv.tv_sec * 1e9 + __tv.tv_nsec);
}


#endif
#endif
