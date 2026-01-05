#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include<stdio.h>

#define SHMKEY 9075 /*共享存储标识*/
#define SEMKEY_S 9085
#define SEMKEY_MUTEX 9086 /*信号量标识*/
#define S_LEN 5 /*缓冲区中缓冲区个数*/
#define PRODUCT_LEN 32 /*产品标识长度——一个产品标识：<=32 字符*/

void set_sembuf_struct(struct sembuf *sem,int semnum, int semop,int semflg)
{ /*给信号结构赋值*/
    sem->sem_num=semnum;
    sem->sem_op=semop;
    sem->sem_flg=semflg;
}

int main()
{
    char *addr, end;
    int shmid;//共享区id
    int semid_s, semid_mutex;//信号量id
    struct sembuf sem_tmp;
    
    if((shmid=shmget(SHMKEY,S_LEN*PRODUCT_LEN+2,0777|IPC_CREAT|IPC_EXCL))==-1)
    {
        if(errno==EEXIST)//此共享区
        {
            printf("The Library has been opened!\n");
            printf("Do you want to close the library(Y:yes)? =:");
            scanf("%c", &end);
            
            if(end=='y'||end=='Y')
            {
                shmid=shmget(SHMKEY,S_LEN*PRODUCT_LEN+2,0777);
                if(shmctl(shmid,IPC_RMID,0)< 0) perror("shmctl:");
                semid_mutex = semget(SEMKEY_MUTEX,1, 0777);
                semid_s = semget(SEMKEY_S,1, 0777);
                semctl(semid_mutex,0,IPC_RMID);
                semctl(semid_s,0,IPC_RMID);
            }
        }
        else
            printf("Fail to opened the library!\n");
        return -1;
    }
    
    //处理共享区间
    addr=(char*)shmat(shmid, 0, 0);
    memset(addr,0,S_LEN*PRODUCT_LEN+2);//快照共享区全空间化为0
    shmdt(addr);//写开条件
    
    //创建信号量
    if((semid_mutex=semget(SEMKEY_MUTEX,1, 0777|IPC_CREAT|IPC_EXCL))==-1)
    {
        if (errno == EEXIST) printf("The SEMKEY_MUTEX Has Existed!\n");
        else
            printf("Fail To Create SEMKEY_MUTEX!\n");
        return -1;
    }
    
    if((semid_s = semget(SEMKEY_S,1, 0777|IPC_CREAT|IPC_EXCL))==-1)
    {
        if (errno == EEXIST) printf("The SEMKEY_S Has Existed!\n");
        else
            printf("Fail to Create SEMKEY_S!\n");
        return -1;
    }
    
    //给信号量赋初值
    set_sembuf_struct(&sem_tmp, 0, 5, 0);
    semop(semid_s, &sem_tmp,1);
    set_sembuf_struct(&sem_tmp, 0, 1, 0);
    semop(semid_mutex, &sem_tmp,1);
    return 0;
}
