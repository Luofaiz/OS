#  【操作系统】课程设计：新增linux驱动程序

## 写在前面

请先阅读[https://blog.csdn.net/qq_46640863/article/details/122684580](https://blog.csdn.net/qq_46640863/article/details/122684580?spm=1001.2014.3001.5502)
对linux内核进行编译。
相关实操移步视频

https://www.bilibili.com/video/BV1QX4y1x7Qn/

#### 目录

- [写在前面](https://blog.csdn.net/qq_46640863/article/details/122952706#_0)
- [一、设计内容及具体要求](https://blog.csdn.net/qq_46640863/article/details/122952706#_7)
- [二、设计思想](https://blog.csdn.net/qq_46640863/article/details/122952706#_15)
- [三、 设计实现与源码剖析](https://blog.csdn.net/qq_46640863/article/details/122952706#__22)
- [四、操作流程](https://blog.csdn.net/qq_46640863/article/details/122952706#_198)
- [五、测试与分析](https://blog.csdn.net/qq_46640863/article/details/122952706#_202)
- [附录](https://blog.csdn.net/qq_46640863/article/details/122952706#_222)
- - [Makefile](https://blog.csdn.net/qq_46640863/article/details/122952706#Makefile_223)
  - [zombotany_blkdev.c](https://blog.csdn.net/qq_46640863/article/details/122952706#zombotany_blkdevc_241)



## 一、设计内容及具体要求

新增[Linux驱动](https://so.csdn.net/so/search?q=Linux驱动&spm=1001.2101.3001.7020)程序
增加一个驱动程序（使用内存模拟设备），使用模块编译方式。
要求：
（1）可以动态加载和卸载新的驱动。
（2）通过程序或命令行使用该驱动。
（3）至少能通过该驱动保存256MB的数据，还能将这些数据读取出来。
（4）要重新编译[Linux内核](https://so.csdn.net/so/search?q=Linux内核&spm=1001.2101.3001.7020)，可模仿ramdisk的实现方式。

## 二、设计思想

题目要求使用内存模拟设备增加一个驱动程序。而内存模拟设备可以模仿Ram Disk的实现方式。经查阅相关资料可得知：Ram Disk的功能是将一部分内存挂载（mount）为外存空间（磁盘）的分区进行使用。从用户的视角看，Ram Disk分区就像磁盘的分区一样，也能对文件进行读写。
但是，Ram Disk与真正的磁盘仍然存在一定区别。在虚拟机重启后，Ram Disk分区消失，Ram Disk分区内部的数据也将消失。
Ram Disk也存在自己的意义。若有几个文件需要被频繁的读写，则可以将其放到由内存开辟的Ram Disk上，大大提高了读写的速度。
在本题目中，采取的就是模仿Ram Disk的实现。在第五章节中，将展示模仿Ram Disk的实现能得到的类似于Ram Disk的效果。
Linux系统将所有设备都视作文件，/dev/设备名 不是目录，而类似于指针指向该块设备，不能直接对其进行读写而需要先进行mount挂载的操作。要读写设备中的文件时，需要先把设备的分区挂载到系统中的一个目录上，通过访问该目录来访问设备。

## 三、 设计实现与源码剖析

在设计属于自己的驱动时，需要实现加载模块时的初始化函数即驱动模块的入口函数。还需要实现卸载模块时的函数，即模块的出口函数。同时，也要实现设备自己的请求处理函数。
首先，对该模块的数据结构进行设计。先定义该块的块设备名、主设备号、大小（256*1024*1024bytes，即256MB）、扇区数为9。

```cpp
#define SIMP_BLKDEV_DISKNAME "zombotany_blkdev"
#define SIMP_BLKDEV_DEVICEMAJOR COMPAQ_SMART2_MAJOR 
#define SIMP_BLKDEV_BYTES (256*1024*1024)
#define SECTOR_SIZE_SHIFT 9
```

定义gendisk表示一个简单的磁盘设备、定义该块设备的拥有者、定义块设备的请求队列指针、开辟该块设备的存储空间。

```cpp
static struct gendisk * zombotany_blkdev_disk;
static struct block_device_operations  zombotany_blkdev_fops = { 
    .owner = THIS_MODULE,
};
static struct request_queue * zombotany_blkdev_queue;
unsigned char  zombotany_blkdev_data[SIMP_BLKDEV_BYTES];
```

入口函数与出口函数这两个函数的方法头如下：

```cpp
static int __init _init(void)  
static void __exit _exit(void)  
```

在入口函数中需要实现的功能包括4个步骤。1.申请设备资源。若申请失败，则退出。2.设置设备有关属性。3.初始化请求队列，若失败则退出。4.添加磁盘块设备。
首先，申请设备资源。判断申请是否成功，若失败则退出。

```cpp
     zombotany_blkdev_disk = alloc_disk(1);
    if(! zombotany_blkdev_disk){
        ret = -ENOMEM;
        goto err_alloc_disk;
}
```

接下来，设置设备有关属性。设置设备名、设备号、fops指针、扇区数

```cpp
strcpy( zombotany_blkdev_disk->disk_name,SIMP_BLKDEV_DISKNAME);
     zombotany_blkdev_disk->major = SIMP_BLKDEV_DEVICEMAJOR;
     zombotany_blkdev_disk->first_minor = 0;
     zombotany_blkdev_disk->fops = & zombotany_blkdev_fops;
    set_capacity( zombotany_blkdev_disk, SIMP_BLKDEV_BYTES>>9);
```

初始化请求队列，若失败则退出。

```cpp
     zombotany_blkdev_queue = blk_init_queue( zombotany_blkdev_do_request, NULL);
    if(! zombotany_blkdev_queue){
        ret = -ENOMEM;
        goto err_init_queue;
    }
zombotany_blkdev_disk->queue =  zombotany_blkdev_queue;
```

最后添加磁盘块设备。

```cpp
    add_disk( zombotany_blkdev_disk);
    return 0;
```

模块的出口函数较为简单，只需释放磁盘块设备、释放申请的设备资源、清除请求队列。

```cpp
static void __exit  zombotany_blkdev_exit(void){
    	del_gendisk( zombotany_blkdev_disk);
    	put_disk( zombotany_blkdev_disk);   
    	blk_cleanup_queue( zombotany_blkdev_queue);
}
```

在实现完入口与出口函数后，需要再声明模块出入口。

```cpp
module_init(xxxx_init);
module_exit(xxxx_exit);
```

实现模块的请求处理函数。请求处理函数涉及到的数据结构如下：当前请求、当前请求bio（通用块层用bio来管理一个请求）、当前请求bio的段链表、当前磁盘区域、缓冲区。

```cpp
struct request *req;
struct bio *req_bio;
struct bio_vec *bvec;
char *disk_mem;     
char *buffer;
```

对于某个请求，先判断该请求是否合法。判断请求是否合法的办法就是判断其是否出现了地址越界的情况。

```cpp
if((blk_rq_pos(req)<<SECTOR_SIZE_SHIFT)+blk_rq_bytes(req)>SIMP_BLKDEV_BYTES){
            blk_end_request_all(req, -EIO);
            continue;
        }
```

若请求合法，则获取当前地址位置。

```cpp
disk_mem =zombotany_blkdev_data + (blk_rq_pos(req) << SECTOR_SIZE_SHIFT);
req_bio = req->bio;
```

判断请求类型，处理读请求与写请求的过程大同小异的。在处理读请求时，遍历请求列表，找到缓冲区与bio，将磁盘内容复制到缓冲区。找到磁盘下一区域，然后处理请求队列下一个请求。

```cpp
while(req_bio != NULL){
                for(i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
                    memcpy(buffer, disk_mem, bvec->bv_len);
                    kunmap(bvec->bv_page);
                    disk_mem += bvec->bv_len;
                }
                req_bio = req_bio->bi_next;
            }
            __blk_end_request_all(req, 0);
            break;
```

在处理写请求时，是把缓冲区内容复制到磁盘上。只需在调用memcpy时将两个参数互换即可，其余相同。

```cpp
memcpy(disk_mem, buffer, bvec->bv_len);
```

该部分代码如下：

```cpp
while(req_bio != NULL){
                for(i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
                    memcpy(disk_mem, buffer, bvec->bv_len);
                    kunmap(bvec->bv_page);
                    disk_mem += bvec->bv_len;
                }
                req_bio = req_bio->bi_next;
            }
            __blk_end_request_all(req, 0);
            break;
```

该模块完整代码见附录，文件名为zombotany_blkdev.c

在编写完模块代码后，还需要编写Makefile文件。Linux的文件系统中，文件没有扩展名。Makefile文件没有扩展名。
首先，在第一次读取执行此Makefile时，KERNELRELEASE没有被定义，所以make只会执行else之后的内容。

```cpp
ifneq ($(KERNELRELEASE),)
```

得到内核源码的路径与当前的工作路径

```cpp
KDIR ?= /lib/modules/$(shell uname -r)/build 
	PWD := $(shell pwd)
```

若之前执行过Makefile，则需要清理掉之前编译过的模块。

```cpp
modules:
		$(MAKE) -C $(KDIR) M=$(PWD) modules
	modules_install:
$(MAKE) -C $(KDIR) M=$(PWD) modules_install
clean:
        rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.*
.PHONY:modules modules_install clean
```

生成.o文件

```cpp
else
		obj-m := simp_blkdev.o
	endif
```

Makefile完整代码见附录。

## 四、操作流程

将模块源码zombotany_blkdev.c和Makefile文件放在同一目录下。![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/718b7939faa37cd499962d77c2eb6025.png)
在该目录下打开控制台，输入make。则能正确地生成.o文件和.ko文件
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/edcbedebd207121c6ecb299397570247.png)

## 五、测试与分析

模块编译完成后，首先回到该目录，执行语句insmod zombotany_blkdev.ko，将刚编译完成的zombotany_blkdev.ko模块插入。执行完成后，再执行lsmod，查看当前系统中的块设备列表。可以看到，zombotany_blkdev已经存在，已经被插入过了，且大小为256MB。也可以执行lsblk，查看当前块设备。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/9aba0d46d6b89f98a16a09bfcb4154d3.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/2796dad25fcbdaef6d1c8d528b26d45d.png)
在根目录下的/dev/ 路径中，可以看到zombotany_blkdev已经被插入了。执行ls /dev/
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/34035b5a8441894fbce1663b4666e6c8.png)
在插入完成后，需要对该模块进行格式化，建立文件系统。输入mkfs.ext3 /dev/zombotany_blkdev，则在该内存模拟设备上建立了ext3文件系统。![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/69a88e7a98bcb41ed11aa427bbcd630c.png)
在建立完成文件系统后，就可以将该设备挂载到文件系统的目录下。首先需要创建需要挂载的目录。mkdir -p /mnt/temp1。将块设备挂载到该目录下。mount /dev/zombotany_blkdev /mnt/temp1。再运行mount | grep zombotany_blkdev。即挂载完成。
再次执行lsmod，查看模块被调用的情况。该模块被一个用户调用。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e70862f55cad756289cf0659d017b3c5.png)
执行ls/mnt/temp1/ 则看到当前块设备有且只有一个文件：lost+found文件。将当前目录所有文件都复制到块设备上，例如当前在该模块的源代码文件夹目录上。执行cp ./* /mnt/temp1/完成复制，再查看当前块设备文件名单。执行ls /mnt/temp1/，则可以看到该块设备被正确地写入了文件，并可以被读取到。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/11b0ebef83654da171bf55fef9b8d268.png)
执行df -H，则查看当前各个设备使用情况。新增的设备zombotany_blkdev已用了2.9MB。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/3af2952b3b014561abc05ea1226ae172.png)
执行vi /mnt/temp1/zombotany_blkdev.c，还能读取这个文件。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/06b8b751a6e6a1ab278c4cb3e556a8b2.png)
最后对该模块进行卸载。首先删除该目录内所有文件。rm -rf /mnt/temp1/*。
先取消挂载。执行umount /mnt/temp1后执行lsmod | grep zombotany_blkdev。可以看到，这个256MB大小的设备被0个用户调用。
执行rmmod zombotany_blkdev。该语句的作用是移除该模块。运行完成后，再次执行lsmod grep zombotany_blkdev。可以在控制台上看到系统并没有任何输出。说明：zombotany_blkdev模块已经彻底被移除了。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/04c6e3148cbea7a08cd6e81d468f2b3d.png)

## 附录

### Makefile

```cpp
ifeq ($(KERNELRELEASE),)
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
clean:
	rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.*
.PHONY:modules modules_install clean
else
	obj-m := zombotany_blkdev.o
endif
```

### zombotany_blkdev.c

```cpp
#include <linux/module.h>
#include <linux/blkdev.h>

#define SIMP_BLKDEV_DISKNAME "zombotany_blkdev"//设备名称
#define SIMP_BLKDEV_DEVICEMAJOR COMPAQ_SMART2_MAJOR //主设备号
#define SIMP_BLKDEV_BYTES (256*1024*1024)            // 块设备大小为256MB
#define SECTOR_SIZE_SHIFT 9//9个扇区

static struct gendisk * zombotany_blkdev_disk;// gendisk结构表示一个简单的磁盘设备
static struct block_device_operations  zombotany_blkdev_fops = { 
    .owner = THIS_MODULE,//设备主体
};
static struct request_queue * zombotany_blkdev_queue;//指向块设备请求队列的指针
unsigned char  zombotany_blkdev_data[SIMP_BLKDEV_BYTES];// 虚拟磁盘块设备的存储空间

//请求处理函数
static void  zombotany_blkdev_do_request(struct request_queue *q){
    struct request *req;// 正在处理的请求队列中的请求
    struct bio *req_bio;// 当前请求的bio
    struct bio_vec *bvec;// 当前请求的bio的段(segment)链表
    char *disk_mem;      // 需要读/写的磁盘区域
    char *buffer;        // 磁盘块设备的请求在内存中的缓冲区

    while((req = blk_fetch_request(q)) != NULL){//得到请求
        // 判断当前请求是否合法
        if((blk_rq_pos(req)<<SECTOR_SIZE_SHIFT) + blk_rq_bytes(req) > SIMP_BLKDEV_BYTES){//判断地址是否越界访问
            printk(KERN_ERR SIMP_BLKDEV_DISKNAME":bad request:block=%llu, count=%u\n",(unsigned long long)blk_rq_pos(req),blk_rq_sectors(req));//越界访问了，则输出
            blk_end_request_all(req, -EIO);
            continue;//获取下一请求
        }
        //获取需要操作的内存位置
        disk_mem =  zombotany_blkdev_data + (blk_rq_pos(req) << SECTOR_SIZE_SHIFT);
        req_bio = req->bio;// 获取当前请求的bio

        switch (rq_data_dir(req)) {  //判断请求的类型
        case READ:
            // 遍历req请求的bio链表
            while(req_bio != NULL){
                //　for循环处理bio结构中的bio_vec结构体数组（bio_vec结构体数组代表一个完整的缓冲区）
                for(int i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
                    memcpy(buffer, disk_mem, bvec->bv_len);//把内存中数据复制到缓冲区
                    kunmap(bvec->bv_page);
                    disk_mem += bvec->bv_len;
                }
                req_bio = req_bio->bi_next;//请求链表下一个项目
            }
            __blk_end_request_all(req, 0);//被遍历完了
            break;
        case WRITE:
            while(req_bio != NULL){
                for(int i=0; i<req_bio->bi_vcnt; i++){
                    bvec = &(req_bio->bi_io_vec[i]);
                    buffer = kmap(bvec->bv_page) + bvec->bv_offset;
                    memcpy(disk_mem, buffer, bvec->bv_len);//把缓冲区中数据复制到内存
                    kunmap(bvec->bv_page);
                    disk_mem += bvec->bv_len;
                }
                req_bio = req_bio->bi_next;//请求链表下一个项目
            }
            __blk_end_request_all(req, 0);//请求链表遍历结束
            break;
        default:
            /* No default because rq_data_dir(req) is 1 bit */
            break;
        }
    }
}


//模块入口函数
static int __init  zombotany_blkdev_init(void){
    int ret;

    //添加设备之前，先申请设备的资源
     zombotany_blkdev_disk = alloc_disk(1);
    if(! zombotany_blkdev_disk){
        ret = -ENOMEM;
        goto err_alloc_disk;
    }

    //设置设备的有关属性(设备名，设备号，fops指针
    strcpy( zombotany_blkdev_disk->disk_name,SIMP_BLKDEV_DISKNAME);
     zombotany_blkdev_disk->major = SIMP_BLKDEV_DEVICEMAJOR;
     zombotany_blkdev_disk->first_minor = 0;
     zombotany_blkdev_disk->fops = & zombotany_blkdev_fops;
    //将块设备请求处理函数的地址传入blk_init_queue函数，初始化一个请求队列
     zombotany_blkdev_queue = blk_init_queue( zombotany_blkdev_do_request, NULL);
    if(! zombotany_blkdev_queue){
        ret = -ENOMEM;
        goto err_init_queue;
    }
     zombotany_blkdev_disk->queue =  zombotany_blkdev_queue;
	//初始化扇区数
    set_capacity( zombotany_blkdev_disk, SIMP_BLKDEV_BYTES>>9);

    //入口处添加磁盘块设备
    add_disk( zombotany_blkdev_disk);
    return 0;

    err_alloc_disk:
        return ret;
    err_init_queue:
        return ret;
}


//模块的出口函数
static void __exit  zombotany_blkdev_exit(void){
// 释放磁盘块设备
    del_gendisk( zombotany_blkdev_disk);
// 释放申请的设备资源
    put_disk( zombotany_blkdev_disk);   
// 清除请求队列
    blk_cleanup_queue( zombotany_blkdev_queue);
}


module_init( zombotany_blkdev_init);// 声明模块的入口
module_exit( zombotany_blkdev_exit);// 声明模块的出口
```
