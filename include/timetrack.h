#ifndef TIME_TRACK
#define TIME_TRACK

#ifndef STOP_BENCH

#include<sys/ipc.h>
#include<sys/shm.h>

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#define TIME_COLUM 12
#define NUM_ROW 1000

typedef struct timeTrack{
    int eventID;
    int valid;
    long long array[TIME_COLUM];
}timeTrack;

long long gettime_nanoTime(void)
{
    struct timespec __tv;
    clock_gettime(CLOCK_MONOTONIC,&__tv);
    //clock_gettime(CLOCK_REALTIME,&__tv);
    return(long long)(__tv.tv_sec * 1e9 + __tv.tv_nsec);
}

struct fd_pair{
   pid_t pid;    // the file name is /tmp/vgl/pid
   FILE *fd;
   int   status; //0 closed, 1 open.
   struct fd_pair *next;
};


#endif
#endif
