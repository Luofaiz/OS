#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SHMKEY 12345
#define SEMKEY_S 12346
#define SEMKEY_MUTEX 12347
#define S_LEN 5
#define MAGIC_NUMBER 0x12345678

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

int V(int semid) {
    struct sembuf v_buf;
    v_buf.sem_num = 0;
    v_buf.sem_op = 1;
    v_buf.sem_flg = 0;
    if(semop(semid, &v_buf, 1) == -1) {
        perror("V操作失败");
        return -1;
    }
    return 0;
}

int P(int semid) {
    struct sembuf p_buf;
    p_buf.sem_num = 0;
    p_buf.sem_op = -1;
    p_buf.sem_flg = 0;
    if(semop(semid, &p_buf, 1) == -1) {
        perror("P操作失败");
        return -1;
    }
    return 0;
}

void display_library_status(LibraryStatus *status) {
    printf("\n========== 当前阅览室状态 ==========\n");
    printf("当前读者：%d/5 人\n", status->current_readers);
    printf("累计访问：%d 人次\n\n", status->total_visitors);
    
    printf("座位分配情况：\n");
    printf("+----+----------+--------------+------------+\n");
    printf("|座位|   状态   |    姓名      |   学号     |\n");
    printf("+----+----------+--------------+------------+\n");
    
    for(int i = 0; i < S_LEN; i++) {
        printf("| %d  |", status->seats[i].seat_number);
        if(status->seats[i].is_occupied) {
            printf("   占用   |%-14s|%-12s|\n", 
                   status->seats[i].name, 
                   status->seats[i].student_id);
        } else {
            printf("   空闲   |      ----    |    ----    |\n");
        }
    }
    printf("+----+----------+--------------+------------+\n");
}

int main() {
    LibraryStatus *library_status;
    int shmid, semid_s, semid_mutex;
    int cleared_users = 0;
    
    printf("开始清理所有测试用户...\n");
    
    // 获取共享内存和信号量
    shmid = shmget(SHMKEY, sizeof(LibraryStatus), 0777);
    semid_mutex = semget(SEMKEY_MUTEX, 1, 0777);
    semid_s = semget(SEMKEY_S, 1, 0777);
    
    if(shmid == -1 || semid_mutex == -1 || semid_s == -1) {
        printf("错误：无法连接到阅览室系统\n");
        printf("请先运行 './library' 初始化阅览室\n");
        exit(1);
    }
    
    // 验证系统完整性
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    if(library_status == (void*)-1) {
        printf("错误：共享内存附加失败\n");
        exit(1);
    }
    
    if(library_status->magic_number != MAGIC_NUMBER) {
        printf("错误：阅览室系统数据错误\n");
        shmdt(library_status);
        exit(1);
    }
    
    printf("成功连接到阅览室系统\n");
    
    // 显示清理前状态
    printf("\n清理前状态：");
    display_library_status(library_status);
    int initial_readers = library_status->current_readers;
    shmdt(library_status);
    
    if(initial_readers == 0) {
        printf("\n阅览室已经是空的，无需清理\n");
        return 0;
    }
    
    // 进入临界区清理所有用户
    P(semid_mutex);
    
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    
    printf("\n开始清理用户：\n");
    for(int i = 0; i < S_LEN; i++) {
        if(library_status->seats[i].is_occupied) {
            printf("清理用户：%s (座位%d)\n", 
                   library_status->seats[i].name, i + 1);
            
            // 清空座位信息
            library_status->seats[i].is_occupied = 0;
            strcpy(library_status->seats[i].name, "");
            strcpy(library_status->seats[i].phone, "");
            strcpy(library_status->seats[i].student_id, "");
            library_status->seats[i].enter_time = 0;
            
            library_status->current_readers--;
            cleared_users++;
            
            // 释放一个座位资源
            V(semid_s);
        }
    }
    
    printf("\n清理完成，共清理了 %d 个用户\n", cleared_users);
    
    // 显示清理后状态
    printf("\n清理后状态：");
    display_library_status(library_status);
    
    shmdt(library_status);
    V(semid_mutex); // 释放互斥资源
    
    printf("\n阅览室已清空，可以重新开始测试！\n");
    
    return 0;
}
