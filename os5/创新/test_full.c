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

// 测试用户数据
char test_names[5][32] = {"张三", "李四", "王五", "赵六", "孙七"};
char test_phones[5][16] = {"13812345678", "13987654321", "15612345678", "15987654321", "18612345678"};
char test_student_ids[5][16] = {"2021001", "2021002", "2021003", "2021004", "2021005"};

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

int find_available_seat(LibraryStatus *status) {
    for(int i = 0; i < S_LEN; i++) {
        if(!status->seats[i].is_occupied) {
            return i;
        }
    }
    return -1;
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
    int added_users = 0;
    
    printf("开始自动填充测试用户...\n");
    
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
    
    // 显示初始状态
    display_library_status(library_status);
    shmdt(library_status);
    
    // 添加5个测试用户
    for(int i = 0; i < 5; i++) {
        printf("\n正在添加第%d个用户：%s...\n", i+1, test_names[i]);
        
        // 尝试申请座位
        if(P(semid_s) == -1) {
            printf("座位申请失败，可能已满员\n");
            break;
        }
        
        // 进入临界区
        P(semid_mutex);
        
        library_status = (LibraryStatus*)shmat(shmid, 0, 0);
        
        int seat = find_available_seat(library_status);
        if(seat != -1) {
            time_t enter_time = time(NULL);
            
            // 填写用户信息
            library_status->seats[seat].is_occupied = 1;
            strcpy(library_status->seats[seat].name, test_names[i]);
            strcpy(library_status->seats[seat].phone, test_phones[i]);
            strcpy(library_status->seats[seat].student_id, test_student_ids[i]);
            library_status->seats[seat].enter_time = enter_time;
            
            // 更新统计
            library_status->current_readers++;
            library_status->total_visitors++;
            
            printf("  用户 %s 已分配到 %d 号座位\n", test_names[i], seat + 1);
            added_users++;
        } else {
            printf("  错误：无法为用户 %s 分配座位\n", test_names[i]);
            V(semid_s); // 释放座位资源
        }
        
        shmdt(library_status);
        V(semid_mutex); // 释放互斥资源
        
        // 添加小延迟，模拟真实情况（使用sleep代替usleep）
        sleep(1); // 1秒
    }
    
    // 显示最终状态
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    printf("\n======== 填充完成 ========\n");
    printf("成功添加 %d 个测试用户\n", added_users);
    display_library_status(library_status);
    
    if(library_status->current_readers == 5) {
        printf("\n阅览室已满员！现在可以测试：\n");
        printf("1. 运行 './read' 测试新用户等待座位的情况\n");
        printf("2. 运行 'make clear_test' 清空所有测试用户\n");
    } else {
        printf("\n还有 %d 个空闲座位\n", 5 - library_status->current_readers);
    }
    
    shmdt(library_status);
    
    return 0;
}
