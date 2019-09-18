#ifndef TIME_TRACK
#define TIME_TRACK

#include<sys/ipc.h>
#include<sys/shm.h>

#include <sys/types.h>
#include <unistd.h>
#define TIME_COLUM 12
#define NUM_ROW 1000

#define SLOT_NUM 1

typedef struct timeTrack{
    int eventID;
    int valid;
    long long array[TIME_COLUM];
}timeTrack;

struct fd_pair{
   pid_t pid;    // the file name is /tmp/vgl/pid
   FILE *fd;
   int   status; //0 closed, 1 open.
   struct fd_pair *next;
};

//Event of intrest queue
//        start
//-->|_|-->|_|-->|_|-->|_|-->
//   tail              head
class EOI_LoopQueue{
    public:
        int head;
        int tail;
        int length;
        int id_array[SLOT_NUM];

    public:
        EOI_LoopQueue(){
           head = 0;
           tail = 0;
           length = 0;
        };
        int empty(){
            return (length==0);
        };
        void push(int EventID){
            if(empty()){
                id_array[head] = EventID;
                length=1;
            }else{
                head += 1;
                head %= SLOT_NUM;
                id_array[head] = EventID;
                length+=1;
                if(head == tail){
                    tail += 1;
                    tail %= SLOT_NUM;
                    length = SLOT_NUM;
                }
                if(length >= SLOT_NUM) length = SLOT_NUM;
            }
        };
        int pop(){
            if(empty()) return -1;
            else{
                int tmp = id_array[tail];
                tail += 1;
                tail %= SLOT_NUM;
                length -= 1;
                if(length == 0) {
                    tail = head = 0;
                }
                return tmp;
            }
        };
        int get_length(){
            return length;
        };
};

#endif
