#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>

#define SHMKEY 9075
#define SEMKEY_S 9085
#define SEMKEY_MUTEX 9086
#define S_LEN 5
#define PRODUCT_LEN 32

int P(int semid){//P操作
    struct sembuf p_buf;
    p_buf.sem_num = 0;
    p_buf.sem_op = -1;
    p_buf.sem_flg = 0;
    if(semop(semid, &p_buf, 1)==-1){
        perror ("p (semid) failed");
        exit (1);
    }
    else return (0);
}

int V(int semid){//V操作
    struct sembuf v_buf;
    v_buf.sem_num = 0;
    v_buf.sem_op = 1;
    v_buf.sem_flg = 0;
    if(semop(semid, &v_buf, 1)==-1){
        perror (" v (semid) failed");
        exit (1);
    }
    else return (0);
}

main(){
    char *p_buffer;//共享空间地址
    unsigned char nowloc;//当前读者信息的指针
    char product[128];
    char end;
    int shmid;//共享区id
    int semid_s,semid_mutex;//信号量标识id
    int n=0;//当前读者人数
    
    shmid = shmget(SHMKEY, S_LEN * PRODUCT_LEN+2, 0777);//获取共享区id
    semid_mutex = semget(SEMKEY_MUTEX,1, 0777);//获取信号量id
    semid_s = semget(SEMKEY_S,1, 0777);
    
    P(semid_s);//申请系统资源
    P(semid_mutex);//申请互斥资源
    printf("Please enter your name(a string ,length <= 32B):\n====:");
    gets(product);
    V(semid_mutex); //释放互斥资源
    printf("Reader %s is reading\n",product);//读者开始读书信息
    
    p_buffer = (char*)shmat(shmid, 0, 0);//附加入到读者读者信息表入共享区
    nowloc = (unsigned char)(*p_buffer);
    for(;;){
        if(strcmp(p_buffer + 2 + nowloc * PRODUCT_LEN, "")==0)
        {
            strncpy(p_buffer + 2 + nowloc * PRODUCT_LEN, product, PRODUCT_LEN);
            nowloc = (nowloc + 1) % S_LEN;
            break;
        }
        nowloc = (nowloc + 1) % S_LEN;
    }
    
    *p_buffer = (char)nowloc; shmdt(p_buffer);//写开条件
    printf("Now Readers in the Library:\n");//显示所有正在读书读者
    p_buffer = (char*)shmat(shmid, 0, 0);
    nowloc = (unsigned char)(*p_buffer);
    for(int i=0;i<5;i++){//遍历
        if(strcmp(p_buffer + 2 + nowloc * PRODUCT_LEN,"")==0){
            n++;
            printf("%s",p_buffer + 2 + nowloc * PRODUCT_LEN); printf(" ");
        }
        nowloc = (nowloc + 1) % S_LEN;
    }
    printf("\n");
    printf("The number of readers in the library:");//显示正在读书的读者人数
    printf("%d",n);
    printf("\n");
    *p_buffer = (char)nowloc; shmdt(p_buffer);
    
    //反复询问读者是否需要离开
    for (;;){
        printf("Do You Want To Leave The Library(Y = yes)?\n====:");
        scanf("%c", &end);
        if(end == 'y' || end == 'Y') break;//离开
    }
    
    P(semid_mutex);//申请互斥资源
    printf("Reader ");
    printf("%s",product);
    printf(" has left the library.\n");
    p_buffer = (char*)shmat(shmid, 0, 0);
    nowloc = (unsigned char)(*p_buffer);
    for(;;){//搜索开读者信息从共享区删除
        if (strcmp(product, p_buffer + 2 + nowloc * PRODUCT_LEN)==0){
            strncpy(p_buffer + 2 + nowloc * PRODUCT_LEN, "", PRODUCT_LEN);
            nowloc = (nowloc + 1) % S_LEN;
            break;
        }
        nowloc = (nowloc + 1) % S_LEN;
    }
    *p_buffer = (char)nowloc; shmdt(p_buffer);
    V(semid_mutex);//释放互斥资源
    V(semid_s);//释放对读者空间资源
    shmdt(p_buffer);
}
