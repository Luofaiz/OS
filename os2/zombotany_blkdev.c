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
                int i=0;
                for(i=0; i<req_bio->bi_vcnt; i++){
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
                int i=0;
                for(i=0; i<req_bio->bi_vcnt; i++){
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

