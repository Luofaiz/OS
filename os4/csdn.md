#  内核编译-系统缺页次数统计实验流程记录（基于CentOS 7.5）

## 写在前面

经过两天的反复实验，磕磕绊绊终于完成了一个完整的成功案例，编译内核时间长达两到三个小时，而且很容易出现错误，所以提示大家做好系统备份（VMware备份很简单，拍摄快照即可，建议在关机状态下）；

还有一些基础的工具，比如`gcc，openssl-devel`这些并没有提示安装，如果是新系统的话也请检查是否安装了这些必要工具；

下载源码包网速不佳的可以找FQ插件（时好时坏吧，昨天不行，今天试了还可以）；

**要找比自己系统内核高一点的内核做实验；**

## 我的实验环境

VMware 14 Pro虚拟机

系统版本：CentOS Linux release 7.5.1804 (Core)

内核版本：3.10.0-957.10.1.el7.x86_64

Linux版本：Red Hat 4.8.5-36

## 一、系统准备

### 磁盘容量要求

推荐可用容量20G+，最低不小于15G，具体查看方法

```shell
#输入命令
df -h
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b2dbce731db143495e1f30ab8c7c3685.png)
如果容量不够，先进行扩容或重新安装操作系统并分配更多的磁盘空间；

### 克隆系统

由于编译更换内核比较复杂，具有一定”危险性“，所以如果是使用VMware虚拟机，可以先克隆系统环境，在克隆出来的系统下做此实验，不会对原系统造成影响；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/963883962263f4ddce9f2e94546e41fc.png)

建议在关机状态下进行克隆操作，右键点击目标虚拟机，选择`管理->克隆`；

### 克隆源选择

如果是拍摄过快照（进行过备份），可以从现有快照克隆，如果没有选择第一项即可；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1db389bd22eb486a892e02403b661a52.png)

### 克隆类型选择

两种方式理论上都不会影响到源系统的运行；

磁盘空间足够大的建议创建完整克隆，不用依赖源系统；

链接克隆，占用空间小，但需要依赖源系统存在；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/a02a62a9f371a8606548026157557844.png)

然后输入名称和位置就可以了；

### 查看ip地址

开机登录后，查看ip地址命令

```shell
ip addr
```

得到ip地址后就可以使用其他终端工具连接，获得更好的体验；

## 二、内核源码包准备

### 查看内核版本

首先查看当前系统使用的内核版本，输入命令

```shell
uname -r
#我的是
#3.10.0-957.10.1.el7.x86_64
```

### 下载内核源码包

提示：如果下载速度非常慢，可以挂VPN或者使用浏览器FQ插件提速；

下载方法：https://ieevee.com/tech/2018/03/30/centos-kernel.html

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e986746a29effc3526390a156db0689d.png)

我的内核版本是这里`3.10.0-957.10.1.el7.x86_64`，所以我下载的是`linux-3.10.0-957.12.1.el7.tar.xz`；

（一般选择要比自己的内核版本高一点点的版本）

注意：文件下载下来之后需要重命名带`.tar.xz`后缀，才能进行解压；（不要在Windows下解压）

### 解压内核源码包

首先解压源码包；

```shell
tar -xvJf linux-3.10.0-957.12.1.el7.tar.xz
```

然后将解压后的文件夹移至`/usr/src/`目录下

```shell
mv linux-3.10.0-957.12.1.el7 /usr/src/
```

## 三、修改源码

首先切换到源码主目录，下面的操作全都在主目录下进行

```shell
cd /usr/src/linux-3.10.0-957.12.1.el7/
```

### 定义pfcount变量

这里需要修改`arch/x86/mm/fault.c`文件，由于文件内容较长，我们首先要确定添加大致位置；

```shell
cat -n arch/x86/mm/fault.c | grep __do_page_fault
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/43cbe1958111b5e1e5acc1f96366ab63.png)

根据输出，大致位置在`1114`行（你的可能不一样，以你的输出为主）

vim打开，并按`:`输入`你的行数`，回车跳转；

```shell
vim arch/x86/mm/fault.c
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/5297e0cab2a81211634826c1e1a848d3.png)

即可跳转至目标行

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/46f5ab45c442a0debe945e75e0a9d933.png)

然后在注释前面加入`pfcount`变量的定义

```shell
unsigned long volatile pfcount;
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e51271dbdacb1f310edd897f534bcc20.png)

`:wq`保存退出；

### pfcount变量自增

由于建议将该语句添加在`good_area:`内，所以还是先定位`good_area:`的位置；

```shell
cat -n arch/x86/mm/fault.c | grep good_area:
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/8ebbd796f80de962d3a2a814bd135c19.png)

我的是`1275`行，和前一步一样，vim打开，并跳转至指定行；

```shell
vim arch/x86/mm/fault.c
```

然后加上`pfcount++;`即可；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/2cf2a32283c05abce832d52f6327b4d4.png)

`:wq`保存退出；

### 修改内存管理代码

接着，vim打开`include/linux/mm.h`头文件；

```shell
vim include/linux/mm.h
```

在`mm.h`中加入全局变量`pfcount`的声明——`extern unsigned long volatile pfcount;`

代码加在`extern int page_cluster;`语句之后；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1f32badee664ee23c96e77ada4108440.png)

### 导出pfcount全局变量

我们需要导出pfcount全局变量，以便于让系统所有模块访问；

需要在`kernel/kallsyms.c`文件的最后一行加入`EXPORT_SYMBOL(pfcount);`

可以直接使用命令

```shell
echo 'EXPORT_SYMBOL(pfcount);'>>kernel/kallsyms.c
```

检查是否成功

```shell
cat kernel/kallsyms.c | grep pfcount
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b13780ad8df65d70cebbac57cafde50e.png)

## 四、生成配置文件

首先安装`ncurses-devel elfutils-libelf-devel openssl-devel`这三个工具；

```shell
yum install ncurses-devel elfutils-libelf-devel openssl-devel -y
```

然后如果不是第一次编译，请先执行命令清理（包括后面编译出错，重新编译也要先执行下面的这条命令）

```shell
make mrproper
```

执行命令生成`.config`配置文件

```shell
make menuconfig
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/c6370bb9a24f4296b7f12f8046974b89.png)

进入可视化界面，直接`Save`即可，使用默认配置；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/9bd3c435e85c0d1cd204efe9883e91f4.png)

`OK`生成`.config`配置文件，然后`Exit`退出；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1a85e82335fb59b8fa2506cb97ce4126.png)

配置文件已经成功生成；

## 五、编译与安装

### 内核编译

在编译之前，需要安装`bc`工具（过程中需要）；

执行命令

```shell
yum install bc -y
```

编译直接在主目录下使用`make`命令

```shell
make
```

编译时间较长，请耐心等待；（这个内核编译大概两个半小时，还不算编译模块的时间）；

编译完成后，可以在`arch/x86_64/boot/`下查看到产生的内核文件`bzImage`（32位系统在`arch/x86/boot`下）；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/ab55dbaaceaadbcc6e38aec89e1437d9.png)

### 模块编译

内核编译完之后，执行命令编译模块

```shell
make modules
```

### 安装模块

由于自身编译源码，会有很多debug模块的存在，占用大量存储空间，我们需要在安装的时候排除它，添加参数`INSTALL_MOD_STRIP=1`；

执行命令

```shell
make INSTALL_MOD_STRIP=1 modules_install
```

### 安装内核

执行命令

```shell
make INSTALL_MOD_STRIP=1 install
```

安装成功后，可以查看`/lib/modules`目录下的文件（这里我们安装的内核并没有带次版本号，不影响使用，在后面使用make命令编译的时候就可以看到）

```shell
ls /lib/modules
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/fce896b4200f35b4a21c668554cf5e8d.png)

在虚拟机上执行`reboot`命令，并在重启时候键盘上下建选择我们自己安装的内核，回车进入；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/a72c28c6bf9619b073081550a587644e.png)

执行命令查看当前使用的内核版本

```shell
uname -r
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/6b8e0f14ba879d9f6f7383836fd5083f.png)

## 六、使用pfcount

在家目录新建一个`source`目录，并`cd`进去；

```shell
mkdir source && cd source
```

### 编写readpfcount.c

`readpfcount.c`可以参照第三个实验的最后一个编写，只是需要引入我们定义的`pfcount`，要想使用这个变量，我们必须引入`linux/mm.h`头文件；

参考代码

```c
/*************************************************
 使用seq_file接口实现可读写proc文件
 参考：https://elixir.bootlin.com/linux/v5.0/source/include/linux/proc_fs.h
 *************************************************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
 
extern unsigned long volatile pfcount;

/*5,实现show函数
  作用是将内核数据输出到用户空间
  将在proc file输出时被调用
  */
static int my_proc_show(struct seq_file *m, void *v)
{
    /*这里不能使用printfk之类的函数
      要使用seq_file输出的一组特殊函数
      */
    seq_printf(m, "The pfcount is %ld !\n", pfcount);
    return 0;
}
 
static int my_proc_open(struct inode *inode, struct file *file)
{
    /*4,在open函数中调用single_open绑定seq_show函数指针*/
    return single_open(file, my_proc_show, NULL);
}
 
/*2,填充proc_create函数中调用的flie_operations结构体
  其中my开头的函数为自己实现的函数，
  seq和single开头为内核实现好的函数，直接填充上就行
  open为必须填充函数
  */
static struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_proc_open,
    .release = single_release,
    .read    = seq_read,
    .llseek  = seq_lseek,
};
 
static int __init my_init(void)
{
    struct proc_dir_entry *file;
    //创建父级目录，第二个参数NULL表示在/proc下
    struct proc_dir_entry *parent = proc_mkdir("201608040212",NULL);

    /*1,
      首先要调用创建proc文件的函数，需要绑定flie_operations
      参数1：要创建的文件
      参数2：权限设置
      参数3：父级目录，如果传NULL，则在/proc下
      参数4：绑定flie_operations
      */
    file = proc_create("readpfcount", 0644, parent, &my_fops);
    if(!file)
        return -ENOMEM;
    return 0;
}
 
/*6,删除proc文件*/
static void __exit my_exit(void)
{
  //移除目录及文件
  remove_proc_entry("201608040212", NULL);
}
 
module_init(my_init);
module_exit(my_exit);
```

### 编写Makefile

`Makefile`参考内容

```shell
ifneq ($(KERNELRELEASE),)
	obj-m:=readpfcount.o
else
	KDIR:= /lib/modules/$(shell uname -r)/build
	PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif
```

### 编译

目录内容

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/ed3d08aa2b5412b11311c99fb699d396.png)

输入`make`编译，可以看到我们的确是使用的新安装的内核

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b03007346b1567b104afdb9173eb93d3.png)

### 安装模块

输入命令

```shell
insmod readpfcount.ko
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/eb1f0753d49108a7b4a7fb38c8634c68.png)

### 查看pfcount的值

输入命令

```shell
cat /proc/${你的学号}/readpfcount
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b60529e4a12ec007078ca33e4b033443.png)

## 后记

虚拟机存储扩展方面，如果在安装操作系统的时候使用的是自动分区的方式（文件系统格式一般为lvm），那么一般使用LVM都会成功；

如果是自己手动分区那么先查看下自己选择的文件系统的格式，具体方法：

在root用户的家目录下都会有`anaconda-ks.cfg`文件，这个文件是记录了你在安装系统时的选择；

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/74d43f168aaaa89d1adc9acc0b1de31b.png)

`cat`查看这个文件

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/f791c6f804492f49547e23daafa7841e.png)

可见我的这个文件系统是`ext4`格式；

如果是自动分区的方式，`type`一般为`lvm`：

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e5ceedcd69ea4828c70bc12636549499.png)

![img](https://g.csdnimg.cn/side-toolbar/3.6/images/totop.png)
