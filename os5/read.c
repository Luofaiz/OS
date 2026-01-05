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

int P(int semid) {
    struct sembuf p_buf;
    p_buf.sem_num = 0;
    p_buf.sem_op = -1;
    p_buf.sem_flg = 0;
    if(semop(semid, &p_buf, 1) == -1) {
        perror("P操作失败");
        exit(1);
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
        exit(1);
    }
    return 0;
}

void display_library_status(LibraryStatus *status) {
    printf("\n========== 阅览室实时状态 ==========\n");
    printf("开放时间：%s", ctime(&status->library_open_time));
    printf("当前读者：%d/5 人\n", status->current_readers);
    printf("累计访问：%d 人次\n\n", status->total_visitors);
    
    printf("座位分配情况：\n");
    printf("+----+----------+--------------+------------+--------------+\n");
    printf("|座位|   状态   |    姓名      |   学号     |   入馆时间   |\n");
    printf("+----+----------+--------------+------------+--------------+\n");
    
    for(int i = 0; i < S_LEN; i++) {
        printf("| %d  |", status->seats[i].seat_number);
        if(status->seats[i].is_occupied) {
            printf("   占用   |%-14s|%-12s|", 
                   status->seats[i].name, 
                   status->seats[i].student_id);
            
            time_t now = time(NULL);
            int duration = (int)(now - status->seats[i].enter_time);
            printf(" %02d:%02d:%02d   |\n", 
                   duration/3600, (duration%3600)/60, duration%60);
        } else {
            printf("   空闲   |      ----    |    ----    |     ----     |\n");
        }
    }
    printf("+----+----------+--------------+------------+--------------+\n");
}

int validate_phone(char *phone) {
    if(strlen(phone) != 11) return 0;
    for(int i = 0; i < 11; i++) {
        if(phone[i] < '0' || phone[i] > '9') return 0;
    }
    return 1;
}

int validate_student_id(char *student_id) {
    return strlen(student_id) >= 6 && strlen(student_id) <= 15;
}

int find_available_seat(LibraryStatus *status) {
    for(int i = 0; i < S_LEN; i++) {
        if(!status->seats[i].is_occupied) {
            return i;
        }
    }
    return -1;
}

int main() {
    LibraryStatus *library_status;
    char name[32], phone[16], student_id[16];
    char choice;
    int shmid, semid_s, semid_mutex;
    int my_seat = -1;
    time_t enter_time;
    
    printf("欢迎来到智能阅览室系统！\n");
    
    // 获取共享内存和信号量
    shmid = shmget(SHMKEY, sizeof(LibraryStatus), 0777);
    semid_mutex = semget(SEMKEY_MUTEX, 1, 0777);
    semid_s = semget(SEMKEY_S, 1, 0777);
    
    if(shmid == -1) {
        printf("错误：无法连接到阅览室系统（共享内存）\n");
        printf("请先运行 './library' 初始化阅览室\n");
        exit(1);
    }
    
    if(semid_mutex == -1 || semid_s == -1) {
        printf("错误：无法连接到阅览室系统（信号量）\n");
        printf("请先运行 './library' 初始化阅览室\n");
        exit(1);
    }
    
    // 验证系统完整性
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    if(library_status == (void*)-1) {
        printf("错误：共享内存附加失败: %s\n", strerror(errno));
        exit(1);
    }
    
    if(library_status->magic_number != MAGIC_NUMBER) {
        printf("错误：阅览室系统数据错误，请重新初始化\n");
        shmdt(library_status);
        exit(1);
    }
    
    printf("成功连接到阅览室系统\n");
    
    // 显示当前状态
    display_library_status(library_status);
    shmdt(library_status);
    
    // 申请座位
    printf("\n正在为您申请座位...\n");
    P(semid_s); // 申请座位资源
    
    printf("座位申请成功！请填写入馆信息：\n");
    
    // 输入读者信息
    printf("请输入姓名：");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;
    
    do {
        printf("请输入手机号（11位）：");
        fgets(phone, sizeof(phone), stdin);
        phone[strcspn(phone, "\n")] = 0;
        if(!validate_phone(phone)) {
            printf("错误：手机号格式错误！请输入11位数字。\n");
        }
    } while(!validate_phone(phone));
    
    do {
        printf("请输入学号（6-15位）：");
        fgets(student_id, sizeof(student_id), stdin);
        student_id[strcspn(student_id, "\n")] = 0;
        if(!validate_student_id(student_id)) {
            printf("错误：学号格式错误！请输入6-15位字符。\n");
        }
    } while(!validate_student_id(student_id));
    
    // 注册操作（互斥区）
    P(semid_mutex);
    
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    
    my_seat = find_available_seat(library_status);
    if(my_seat != -1) {
        enter_time = time(NULL);
        
        // 填写座位信息（注册）
        library_status->seats[my_seat].is_occupied = 1;
        strcpy(library_status->seats[my_seat].name, name);
        strcpy(library_status->seats[my_seat].phone, phone);
        strcpy(library_status->seats[my_seat].student_id, student_id);
        library_status->seats[my_seat].enter_time = enter_time;
        
        // 更新统计
        library_status->current_readers++;
        library_status->total_visitors++;
        
        printf("\n注册成功！\n");
        printf("读者：%s\n", name);
        printf("座位号：%d\n", my_seat + 1);
        printf("入馆时间：%s", ctime(&enter_time));
    }
    
    shmdt(library_status);
    V(semid_mutex); // 释放互斥资源
    
    if(my_seat == -1) {
        printf("错误：无法分配座位！\n");
        V(semid_s);
        exit(1);
    }
    
    // 阅读菜单
    while(1) {
        printf("\n您正在 %d 号座位阅读...\n", my_seat + 1);
        printf("+---------------------------------+\n");
        printf("|         阅读菜单选项            |\n");
        printf("+---------------------------------+\n");
        printf("| s - 查看阅览室状态              |\n");
        printf("| m - 查看我的信息                |\n");
        printf("| r - 继续阅读                    |\n");
        printf("| q - 退出阅览室                  |\n");
        printf("+---------------------------------+\n");
        printf("请选择操作 (s/m/r/q)：");
        
        scanf(" %c", &choice);
        
        switch(choice) {
            case 's':
            case 'S':
                library_status = (LibraryStatus*)shmat(shmid, 0, 0);
                display_library_status(library_status);
                shmdt(library_status);
                break;
                
            case 'm':
            case 'M':
                library_status = (LibraryStatus*)shmat(shmid, 0, 0);
                printf("\n您的信息：\n");
                printf("   姓名：%s\n", library_status->seats[my_seat].name);
                printf("   学号：%s\n", library_status->seats[my_seat].student_id);
                printf("   手机：%s\n", library_status->seats[my_seat].phone);
                printf("   座位：%d号\n", my_seat + 1);
                
                time_t now = time(NULL);
                int duration = (int)(now - library_status->seats[my_seat].enter_time);
                printf("   已阅读：%d小时%d分钟%d秒\n", 
                       duration/3600, (duration%3600)/60, duration%60);
                shmdt(library_status);
                break;
                
            case 'r':
            case 'R':
                printf("继续享受阅读时光...\n");
                sleep(2);
                break;
                
            case 'q':
            case 'Q':
                goto exit_library;
                
            default:
                printf("无效选择，请重新输入！\n");
        }
    }
    
exit_library:
    // 注销操作（互斥区）
    P(semid_mutex);
    
    library_status = (LibraryStatus*)shmat(shmid, 0, 0);
    
    time_t leave_time = time(NULL);
    int total_duration = (int)(leave_time - library_status->seats[my_seat].enter_time);
    
    printf("\n读者 %s 正在离开阅览室...\n", name);
    printf("本次阅读统计：\n");
    printf("   阅读时长：%d小时%d分钟%d秒\n", 
           total_duration/3600, (total_duration%3600)/60, total_duration%60);
    printf("   使用座位：%d号\n", my_seat + 1);
    
    // 清空座位信息（注销）
    library_status->seats[my_seat].is_occupied = 0;
    strcpy(library_status->seats[my_seat].name, "");
    strcpy(library_status->seats[my_seat].phone, "");
    strcpy(library_status->seats[my_seat].student_id, "");
    library_status->current_readers--;
    
    printf("注销成功！感谢您的使用，欢迎下次光临！\n");
    
    shmdt(library_status);
    V(semid_mutex); // 释放互斥资源
    V(semid_s);     // 释放座位资源
    
    return 0;
}
