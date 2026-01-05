#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SHMKEY 12345
#define SEMKEY_S 12346
#define SEMKEY_MUTEX 12347
#define S_LEN 5

typedef struct {
    char name[32];          
    char phone[16];         
    char student_id[16];    
    time_t enter_time;      
    int seat_number;        
    int is_occupied;        
} ReaderInfo;

typedef struct {
    int total_visitors;     
    int current_readers;    
    ReaderInfo seats[S_LEN]; 
    time_t library_open_time; 
    int magic_number;
} LibraryStatus;

#define MAGIC_NUMBER 0x12345678

void set_sembuf_struct(struct sembuf *sem, int semnum, int semop, int semflg)
{
    sem->sem_num = semnum;
    sem->sem_op = semop;
    sem->sem_flg = semflg;
}

void init_library_data(LibraryStatus *library_status) {
    memset(library_status, 0, sizeof(LibraryStatus));
    
    for(int i = 0; i < S_LEN; i++) {
        library_status->seats[i].seat_number = i + 1;
        library_status->seats[i].is_occupied = 0;
        strcpy(library_status->seats[i].name, "");
        strcpy(library_status->seats[i].phone, "");
        strcpy(library_status->seats[i].student_id, "");
    }
    
    library_status->library_open_time = time(NULL);
    library_status->total_visitors = 0;
    library_status->current_readers = 0;
    library_status->magic_number = MAGIC_NUMBER;
}

int init_semaphores() {
    int semid_s, semid_mutex;
    struct sembuf sem_tmp;
    
    // 创建座位信号量
    semid_s = semget(SEMKEY_S, 1, 0777|IPC_CREAT|IPC_EXCL);
    if(semid_s == -1) {
        if (errno == EEXIST) {
            semid_s = semget(SEMKEY_S, 1, 0777);
            semctl(semid_s, 0, IPC_RMID);
            semid_s = semget(SEMKEY_S, 1, 0777|IPC_CREAT);
        } else {
            printf("错误：创建座位信号量失败: %s\n", strerror(errno));
            return -1;
        }
    }
    
    set_sembuf_struct(&sem_tmp, 0, S_LEN, 0);
    if(semop(semid_s, &sem_tmp, 1) == -1) {
        printf("错误：设置座位信号量失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 创建互斥信号量  
    semid_mutex = semget(SEMKEY_MUTEX, 1, 0777|IPC_CREAT|IPC_EXCL);
    if(semid_mutex == -1) {
        if (errno == EEXIST) {
            semid_mutex = semget(SEMKEY_MUTEX, 1, 0777);
            semctl(semid_mutex, 0, IPC_RMID);
            semid_mutex = semget(SEMKEY_MUTEX, 1, 0777|IPC_CREAT);
        } else {
            printf("错误：创建互斥信号量失败: %s\n", strerror(errno));
            return -1;
        }
    }
    
    set_sembuf_struct(&sem_tmp, 0, 1, 0);
    if(semop(semid_mutex, &sem_tmp, 1) == -1) {
        printf("错误：设置互斥信号量失败: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

int main()
{
    int shmid;
    LibraryStatus *library_status;
    
    printf("正在初始化阅览室系统...\n");
    
    // 清理可能存在的旧资源
    shmid = shmget(SHMKEY, 0, 0);
    if(shmid != -1) {
        shmctl(shmid, IPC_RMID, 0);
    }
    
    int semid_tmp;
    semid_tmp = semget(SEMKEY_S, 0, 0);
    if(semid_tmp != -1) {
        semctl(semid_tmp, 0, IPC_RMID);
    }
    
    semid_tmp = semget(SEMKEY_MUTEX, 0, 0);
    if(semid_tmp != -1) {
        semctl(semid_tmp, 0, IPC_RMID);
    }
    
    // 创建共享内存
    shmid = shmget(SHMKEY, sizeof(LibraryStatus), 0777|IPC_CREAT|IPC_EXCL);
    if(shmid == -1) {
        printf("错误：创建共享内存失败: %s\n", strerror(errno));
        return -1;
    }
    
    // 附加共享内存
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    if(library_status == (void*)-1) {
        printf("错误：共享内存附加失败: %s\n", strerror(errno));
        shmctl(shmid, IPC_RMID, 0);
        return -1;
    }
    
    // 初始化数据
    init_library_data(library_status);
    shmdt(library_status);
    
    // 初始化信号量
    if(init_semaphores() != 0) {
        printf("错误：信号量初始化失败\n");
        shmctl(shmid, IPC_RMID, 0);
        return -1;
    }
    
    printf("阅览室系统准备就绪！\n");
    printf("总座位数：%d\n", S_LEN);
    printf("运行 './read' 启动读者进程\n");
    
    return 0;
}
