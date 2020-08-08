#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<time.h>
#include<unistd.h>
#include<string.h>
#include<erron.h>

#define SHM_ID "/mmap-test"
#define PAGE_SIZE 4096
#define BUFFER_SIZE 1024
#define MAX_SEQ_NO 100000
#define SLEEP_NANOS 1000

struct mesg{
    int m_id;
    char m_data[128];
};

struct ring_buffer{
    size_t head;
    size_t tail;
    struct mesg buffer[BUFFER_SIZE];
};

struct ring_buffer *prb;
size_t shmsize=((sizeof(struct ring_buffer)+PAGE_SIZE-1)/PAGE_SIZE)*PAGE_SIZE;
struct timespec tss={0,SLEEP_NANOS};
extern int errno;

void cleanup(void){
    munmap(prb,shmsize);printf("munmap() called\n");
    shm_unlink(SHM_ID); printf("shm_unlink() called\n");
}

void producerLoop(){
    struct mesg *pmsg;
    int fd,seq;

    printf("producerLoop() : shmsize=%d\n",(int)shmsize);
    if((fd=shm_open(SHM_ID,O_RDWR|O_CREAT,6000))<0){
        exit(1);
    }
    if(ftruncate(fd,shmsize)<0){
        exit(1);
    }
    prb=(struct ring_buffer *)mmap(0,shmsize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(prb==MAP_FAILED){
        exit(1);
    }
    close(fd);

    if(atexit(cleanup)<0){
        exit(1);
    }

    seq=prb->tail=prb->head=0;
    pmsg=&prb->buffer[prb->tail%BUFFER_SIZE];
}