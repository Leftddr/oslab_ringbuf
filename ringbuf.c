// http://stackoverflow.com/questions/16283517/single-producer-consumer-ring-buffer-in-shared-memory

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

#define SHM_ID "/mmap-test"
#define PAGE_SIZE 4096
#define BUFFER_SIZE 1024
#define MAX_SEQ_NO 100000
#define SLEEP_NANOS 1000   // 1 microsecond

//sem_t sem;
sem_t *empty;
sem_t *full;

struct mesg {
  int m_id;
  char m_data[128];
};

struct ring_buffer {
  size_t head;
  size_t tail;
  struct mesg buffer[BUFFER_SIZE]; //struct in struct mesg 1024
};

struct ring_buffer *prb;
size_t shmsize = ((sizeof(struct ring_buffer)+PAGE_SIZE-1)/PAGE_SIZE)*PAGE_SIZE;
struct timespec tss = {0, SLEEP_NANOS};
extern int errno; //extern declared

void cleanup(void) {
  munmap(prb, shmsize); printf("munmap() called\n");
  shm_unlink(SHM_ID); printf("shm_unlink() called\n");
}

void producerLoop()
{
  struct mesg *pmsg;
  int fd, seq;

  printf("producerLoop(): shmsize = %d\n", (int)shmsize);
  if ((fd = shm_open(SHM_ID, O_RDWR | O_CREAT, 0600)) < 0) { //shm_open : 공유 메모리를 지정하고 그 안에서 관리한다. "/mmap_test"가 공유 메모리의 이름이다.
    fprintf(stderr, "shm_open(): %s\n", strerror(errno)); // stderr : 파일 구조체 포인터, strerror : 시스템 오류번호에 대한 오류 메세지를 문자열로 반환 "" : 그 파일 안에 쓰는
    exit(1);
  }
  if (ftruncate(fd, shmsize) < 0) {//open 된 fd의 파일을 shmsize만큼 자른다. 만약 이것보다 작으면 0x00으로 채운다.
    fprintf(stderr, "ftruncate(): %s\n", strerror(errno));
    exit(1);
  }
  prb = (struct ring_buffer *) //prb = struct ring buffer를 가리킬 수 있는 구조체 포인터

    mmap(0, shmsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);//mmap = shmsize의 크기만큼 mapping한다.
  if (prb == MAP_FAILED) { // mapping이 실패한 경우
    fprintf(stderr, "mmap(): %s\n", strerror(errno));
    exit(1);
  }
  close(fd);

  if (atexit(cleanup) < 0) { //프로그램이 종료하거나 exit()함수에 의해 종료될때 atexit에 있는 함수를 실행한다.
    fprintf(stderr, "atexit(): %s\n", strerror(errno));
    exit(1);
  }

  seq = prb->tail = prb->head = 0; //size_t는 객체의 크기를 나타내기 위해서 쓰이는 16bit unsigned int타입
  while (1) {
     sem_wait(full);
    //while((prb->tail+1)%BUFFER_SIZE != prb->head%BUFFER_SIZE) { //나누기의 나머지를 판단하여 다들어갔는지 판단
     //하나씩 늘려줄 때마다 tail 변수를 가지고 판단한다.
      pmsg = &prb->buffer[prb->tail%BUFFER_SIZE]; //buffer을 하나씩 할당한다.
      //sem_wait(&sem); //consumer에서 m_id값을 잘못 읽어올수 있으므로 lock시킨다
      pmsg->m_id = seq++; //m_id를 하나씩 늘려준다. pmsg는 지역변수이다. m_id는 발행된 개수이다.
      //sem_post(&sem); // lock을 푼다.
      printf("[%d]\n", pmsg->m_id); //출력해준다.
      //__sync_fetch_and_add(&prb->tail, 1);
      //sem_wait(&sem);
      prb->tail++; //load 하나로 이미 원자적인 행이다.
      //sem_post(&sem);
      printf("%s\n",pmsg->m_data);
      sem_post(empty);
      if (seq == MAX_SEQ_NO) { //seq가 max와 같으면 종료한다.
	printf("producerLoop(): %d messages transferred.\n", seq);
        sem_close(full);
        sem_unlink("full");
	exit(0);
      }
    }
    nanosleep(&tss, 0);
  //}
}

void consumerLoop()
{
  struct mesg msg;
  int fd, seq, pid;

  printf("consumerLoop(): shmsize = %d\n", (int)shmsize);
  if ((fd = shm_open(SHM_ID, O_RDWR, 0600)) < 0) {
    fprintf(stderr, "shm_open(): %s\n", strerror(errno));
    exit(1);
  }
  
  prb = (struct ring_buffer *)
    mmap(0, shmsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); //prb에 같은 공유메모리를 맵핑한다.
  if (prb == MAP_FAILED) {
    fprintf(stderr, "mmap(): %s\n", strerror(errno));
    exit(1);
  }
  close(fd);

  if (atexit(cleanup) < 0) {
    fprintf(stderr, "atexit(): %s\n", strerror(errno));
    exit(1);
  }
  
  seq = prb->head;//consumer은 head를 가지고 판단한다.
  msg = prb->buffer[seq%BUFFER_SIZE];//채워진 부분을 빼와야 하므로
  pid = msg.m_id - 1;//pid는 채워진 개수를 갱신할때 쓰인다.
  while (1) {
    //while(seq%BUFFER_SIZE != prb->tail%BUFFER_SIZE) {//빼온 개수와 들어간 개수가 같으면 종료한다.
      sem_wait(empty);
      msg = prb->buffer[seq%BUFFER_SIZE]; //msg는 그 prb의 buffer이다.
      seq++;
      if(msg.m_id != pid+1) { //어디선가 정보가 빠져나갓다는 이야기 즉 missing이 난것이다.
		fprintf(stderr, "consumerLoop(): a message missing (%d!=%d)\n", msg.m_id, pid);
		exit(1);
      }
      pid = msg.m_id; //m_id값으로 현재를 판단하므로 producer에서 m_id값을 변경시켜주는데다 lock을 걸어준다.
      printf("[%d]\n", msg.m_id);
      prb->head=(size_t)seq;
      sem_post(full);
      if (seq == MAX_SEQ_NO) {
	printf("consumerLoop(): %d messages received.\n", seq);
        sem_close(empty);
        sem_unlink("empty");
	exit(0);
      }
    }
    //sem_wait(&sem);
    //load하나로 이미 원자적인 행이다.
    //sem_post(&sem);
    //__sync_lock_test_and_set(&prb->head, seq); // avoid updating head one-by-one
    nanosleep(&tss, 0);
  //}
}

int main(int argc, char* argv[])
{
  empty=sem_open("empty",O_CREAT,0777,0);
  full=sem_open("full",O_CREAT,0777,BUFFER_SIZE-1);
  //sem_init(&sem,0,1);
  if (argc != 2) {
    printf("usage: %s {producer|consumer}\n", argv[0]);
    exit(1);
  }

  if (strcmp(argv[1], "consumer") == 0) 
    consumerLoop();
  else if(strcmp(argv[1], "producer") == 0) 
    producerLoop();
  else {
    printf("usage: %s {producer|consumer}\n", argv[0]);
    exit(1);
  }

  exit(0);
}
