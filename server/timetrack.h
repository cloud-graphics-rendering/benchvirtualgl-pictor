#ifndef TIME_TRACK
#define TIME_TRACK

#include<sys/ipc.h>
#include<sys/shm.h>

#include <sys/types.h>
#include <unistd.h>
#define TIME_COLUM 10
#define NUM_ROW 100

typedef struct timeTrack{
    int eventID;
    int valid;
    unsigned int array[TIME_COLUM];
}timeTrack;

struct fd_pair{
   pid_t pid;    // the file name is /tmp/vgl/pid
   FILE *fd;
   int   status; //0 closed, 1 open.
   struct fd_pair *next;
};

#endif
