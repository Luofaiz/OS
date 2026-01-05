# 【Linux】实现基于Ext4的模块化文件系统

## 系统环境

**镜像版本**：CentOS 7.6.1810 x86 64位
**Linux内核版本**：4.14.160
![实验准备](https://i-blog.csdnimg.cn/blog_migrate/815b14bcb6d463eaef414dade2aa6bc7.png)

## 内核安装

### 1).获取超级权限

```c
su
```

输入正确的root密码获取超级权限，以下指令都是在获取超级权限的情况下执行的。

### **2).安装内核编译工具**

```c
yum -y install gcc gcc-c++ ncurses ncurses-devel cmake elfutils-libelf-devel openssl-devel 
```

### **3). 编辑内核配置文件**

将内核源码linux-4.14.160.tar拷贝到/usr/src/目录下，解压缩后进行配置。
**进入文件夹：**

```c
cd /usr/src/
```

**解压缩源码：**

```c
tar -xvf linux-4.14.160.tar
```

**进入源码文件夹：**

```c
cd linux-4.14.160
```

**编辑内核配置文件**：

```c
make menuconfig
```

选择`“General setup”->”Local version”`，输入自定义的本地内核版本号用以区分，以“`-yoching`”为例。然后返回主页面，选择“`File systems`”，确保选项`The Extended 4（ext4）filesystem`前面是`M`，即ext4文件系统为模块化加载，若不是选中按“M”键。最后选择“save”保存`.config`配置文件即可。

### **4).开始编译**

**编译文件（使用4个核心）：**

```c
make -j4
```

编译时间较久，请耐心等待。
编译成功，如下图。
![编译成功](https://i-blog.csdnimg.cn/blog_migrate/55848b929ea4edcab3157fad3f6e59b9.png)
**安装模块：**

```c
make modules_install
```

**安装内核：**

```c
make install 
```

### **5).重启并选择新内核**

```c
reboot
```

在内核引导页面选择`4.14.160-yoching`内核进入操作系统，后面为自己定义的本地内核版本号。

## 新文件系统模块化

拷贝内核源码文件夹下`fs/ext4`文件夹里的所有内容，放到其他位置。
**进入文件夹：**

```c
cd /usr/src/linux-4.14.160/fs
```

**复制到/usr/src：**

```c
cp -r ext4 /usr/src/ext4edit 
```

打开ext4edit文件系统根目录下的`Makefile`文件,修改为：

```c
obj-$(CONFIG_EXT4_FS) += ext4edit.o
ext4edit-y	:= balloc.o bitmap.o block_validity.o dir.o ext4_jbd2.o extents.o \
		extents_status.o file.o fsmap.o fsync.o hash.o ialloc.o \
		indirect.o inline.o inode.o ioctl.o mballoc.o migrate.o \
		mmp.o move_extent.o namei.o page-io.o readpage.o resize.o \
		super.o symlink.o sysfs.o xattr.o xattr_trusted.o xattr_user.o
KERNELDIR:=/usr/src/linux-4.14.160
PWD:=$(shell pwd)
ext4edit-$(CONFIG_EXT4_FS_POSIX_ACL)	+= acl.o
ext4edit-$(CONFIG_EXT4_FS_SECURITY)		+= xattr_security.o
default:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.mod.c *.ko *.symvers
AI构建项目c运行1234567891011121314
```

其中`KERNELDIR`变量为内核源代码位置，`PWD`变量为当前工作目录的绝对路径，也就是Ext4edit文件系统源码所在路径。在`Makefile`文件的最后再加上两行编译命令，用以编译模块和清除编译产生的文件。
接着，找到文件夹里`super.c`，这是文件系统挂载是所要用到的文件。找到其中的结构体类型为`file_system_type`的变量 `ext4_fs_type`，修改其中的`name`字段和函数`MODULE_ALIAS_FS()`的参数为`“ext4edit”`。后者作用是设置模块别名。具体代码如下。

```c
static struct file_system_type ext4_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "ext4edit",
	.mount		= ext4_mount,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("ext4edit");
```

然后找到`sysfs.c`文件，这是与顶层虚拟文件系统相关的文件。找到初始化函数`ext4_init_sysfs（）`，修改里面第二行`kobject_set_name()`里的字段为`“ext4edit”`。具体代码如下。

```c
int __init ext4_init_sysfs(void)
{
	int ret;
	kobject_set_name(&ext4_kset.kobj, "ext4edit");
	ext4_kset.kobj.parent = fs_kobj;
	ret = kset_register(&ext4_kset);
	if (ret)
		return ret;
	ret = kobject_init_and_add(&ext4_feat, &ext4_feat_ktype,
				   NULL, "features");
	if (ret)
		kset_unregister(&ext4_kset);
	else
		ext4_proc_root = proc_mkdir(proc_dirname, NULL);
	return ret;
}
```

至此，基于Ext4的新文件系统`Ext4edit`改写完成。
我们可以修改该文件系统的代码，实现自定义的功能。例如，在该文件系统中添加写缓存提示功能。这个功能需要用到`file.c`文件。找到该文件后在该文件里`ext4_file_write_iter（）`函数，添加一条`printk（）`语句，打印提示信息。该函数用于写文件时延迟分配磁盘空间时，将数据按字节写入页缓存。添加的语句如下。

```c
printk("Method ext4_file_write_iter() in file.c."); 
```

## 模块编译及动态静态加载

### 1） 编译模块

重新打开命令行，并输入`su`命令和root密码获取超级权限。将文件系统的源码拷贝到`/usr/src/`目录下。
**进入模块文件夹：**

```c
cd /usr/src/ext4edit
```

**编译模块：**

```c
make
```

### 2).加载文件系统

```c
insmod ext4edit.ko
```

如出现“`Unknown symbol`“错误，使用`modinfo ext4edit.ko`命令，查看模块信息，信息中有depends一项表示依赖的模块，使用`modprobe`先加载依赖的模块即可。Ext4edit文件系统依赖于`mbcache`和`jbd2`两个模块。

### 3).挂载文件系统

**进入/dev目录：**

```c
cd /dev
```

**创建块设备文件：**

```c
mknod -m 640 yoching b 1 0
```

**格式化块设备文件**

```c
mkfs.ext4 yoching
```

**进入/mnt目录：**

```c
cd /mnt
```

**创建yoching文件夹：**

```c
mkdir yoching
```

**挂载文件系统：**

```c
mount /dev/yoching -t ext4edit /mnt/yoching
```

**查看挂载信息：**

```c
df -T -h
```

执行结果的最后一条显示文件系统类型为`ext4edit`的文件系统已挂载在`/mnt/yoching`目录下，说明挂载完成。

![df -T -h 执行结果](https://i-blog.csdnimg.cn/blog_migrate/dd6982767e74245c0a226b8bc06b2961.png)
进入`/mnt/yoching`目录下，使用“`vim hello.txt`”创建并打开名为`hello.txt`的文本文件，切换为`INSERT`模式写入“`hello`”，并按`ESC`，键入“`：wq`”保存并退出。输入“`dmesg -c`”查看后台信息，查看结果如下图所示。
![dmesg -c 执行结果](https://i-blog.csdnimg.cn/blog_migrate/4953a2f899bb21491f67830ad88229c5.png)

### 4).卸载文件系统

**卸载文件系统：**

```c
umount  /mnt/yoching
```

**移除模块：**

```c
rmmod ext4edit
```

