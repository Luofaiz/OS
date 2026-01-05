#  新增Linux系统调用——计算一个数的三次方

## 新增Linux系统调用

> **本文目录**

- 实验内容及要求
- 实验步骤
  - 下载新内核
  - 修改新内核
  - 编译新内核
- 实验测试
- 总结

------

### 一、实验内容及要求

要求：

- 需要重新编译Linux内核；
- 增加一个Linux的系统调用；
- 另外写一个程序进行调用。

功能：

  计算一个数字的三次方，并打印出来。

### 二、实验步骤

  本篇教程所需的实验环境为VMware Workstation 16 Pro + Ubuntu 20.04 。如果你的电脑不具备Linux环境，或者说并没有配置[Ubuntu系统](https://so.csdn.net/so/search?q=Ubuntu系统&spm=1001.2101.3001.7020)，可以参考博主之前的两篇文章——[《VMware虚拟机安装Ubuntu》](https://blog.csdn.net/weixin_44191535/article/details/105150707)、[《VMware下Ubuntu的基本配置》](https://blog.csdn.net/weixin_44191535/article/details/105151862)。

#### 1.查看内核版本

打开【**终端**】，输入 **uname -r**，查看当前操作系统的内核版本。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/eb03f63f83fd4c3d2bcca77d852411c8.png#pic_center)
此时我们看到的内核版本为 5.15.0-75 。

#### 2.下载新内核

  内核下载的官网为 https://mirrors.edge.kernel.org/pub/linux/kernel/，选择需要下载的版本，下载即可。但是下载的速度真的如蜗牛爬一般啊，这里博主推荐使用国内镜像去下载，博主使用的是北京交通大学的开源镜像。[(点击跳转）](https://mirror.bjtu.edu.cn/kernel/linux/kernel/)
  不建议使用最新的内核，建议使用老一点的内核，这样比较稳定些，不会因为内核的问题而导致编译出错。
  博主选择的内核版本为 5.6.8。

在【**终端**】中输入 **wget https://mirror.bjtu.edu.cn/kernel/linux/kernel/v5.x/linux-5.6.8.tar.xz** ，然后 **回车**。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/dec79ce562706ae039ae3dbb2b78d80f.png#pic_center)
下载完成后，在主目录下便可以看到刚才下载的内核压缩包。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/7fdc86cf03d8ea02a4768e59f9b1b9fe.png#pic_center)

#### 3.解压内核压缩包

在【**终端**】中依次输入以下命令：

- **tar -Jxvf linux-5.6.8.tar.xz**
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/c90ed22dc7fc8bfe91fbd374b1ee7c96.png#pic_center)

------

> 下面的所有命令全在 `~/linux-5.6.8` 目录下执行。

首先转到 `~/linux-5.6.8` 目录下
在【**终端**】执行 **cd linux-5.6.8/**
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/c24dd31fd828b02d1986ee8c8b70a2eb.png#pic_center)

#### 4.新增Linux系统调用

（1）添加调用函数**声明**

【**终端**】输入 **vim include/linux/syscalls.h** ，在 #endif 前输入 `asmlinkage long sys_cube(int num);`
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/0ad05bb123d5c658976560d2d76f8aeb.png#pic_center)

注：
  执行完上面命令后，打开了一个头文件，此时vim处于命令模式，我们可以看见光标，但不可以编辑，移动光标到需要编辑的位置，按键盘上的【 i 】，进入编辑模式，待内容输入完成后，按【Esc】，切换至命令模式，此时输入【:wq】，即完成了保存退出操作。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/ccbe3f3747ce8a9607767563a7b53cd9.png#pic_center)

（2）添加调用函数**定义**

【**终端**】输入 **vim kernel/sys.c** ,在末尾 #endif 后输入如下函数：

```c
SYSCALL_DEFINE1(cube,int,num){
    int result = num*num*num;
    printk("The result is %d.\n",result);
    return result;
}
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/9c790b2701c3d4c78e3d45ff911bcffa.png#pic_center)

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e322099f6a9c576d62262a1ebb7bcbfd.png#pic_center)

（3）注册系统调用号

在【**终端**】输入 **vim arch/x86/entry/syscalls/syscall_64.tbl** ，在syscall_64.tbl文件中添加如下内容：
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1678859f65ac63765bd33def9e94d21d.png#pic_center)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/3c0d834c40187d6f8b6194be1b908265.png#pic_center)
此时添加的自定义函数对应的系统调用号是 439 。

------

上面的过程已经添加了一个新的系统调用，下面的过程则是编译内核并安装新内核，可谓是本文的重点所在。

#### 5.安装编译内核所需的依赖包

打开【**终端**】，依次输入下面命令：

```c
sudo apt install make flex bison libssl-dev libelf-dev libncurses5-dev dwarves -y
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/5ac1a9e2c02d37e5456fbda0b6259bba.png#pic_center)

#### 6.配置内核

（可选）【**终端**】输入 **make mrproper** ，用来清除以前配置，若是第一次配置，则不需执行此命令，博主是第一次编译，因此不执行该条命令。

【**终端**】输入**make menuconfig**，在跳出的界面中依次执行【Save】—>【Ok】—>【Exit】—>【Exit】
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/f2b7694990fcd77ea014afc9dbc0b6e4.png#pic_center)

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/d7472eb75ba038d50d6b0d71f80f3ea3.png#pic_center)

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/094a5150bd97fe9e8ff3ad62b7be4043.png#pic_center)

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/4003f082bb52d3503015683365b875a0.png#pic_center)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/4b85d8850aca07955bf9fcee8d738f45.png#pic_center)

#### 7.编译

查看自己机器处理器的核数，可通过 `cat /proc/cpuinfo` 来查看。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e3e026812e3c39978d4a50bb3c08c72f.png#pic_center)
此处的**processor**后面的数字表示的是处理器编号，只需要查看最大编号便可知道处理器的核数了。或者打开虚拟机设置，可通过查看硬件配置来查看处理器的核数，此处为**8**。

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b4de7e9e6242d04dc7434622930ac014.png#pic_center)

在【**终端**】中键入 **make -jn**（n个线程开始编译），因为本文机器的处理器核数为8，故博主这里的命令是 `make -j8` 随后就是漫长的等待了，等到地老天荒，哈哈哈哈，大概一小时左右！

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/722a79bcdc9b3a309596c5ca080991ee.png#pic_center)
中途可能会报一些错误，我将其归纳在此，[点击跳转](https://blog.csdn.net/weixin_44191535/article/details/106954861#1)。

编译要结束时，如果看到下图所示的信息，说明你已经编译成功了，一定要看到此提示信息，因为有时候会内核编译停止，重启电脑后可能开不了机器。
若没有看到此条信息，建议继续`make -j8`一下。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/d6318691b60ce60e76ba27b30cb36448.png#pic_center)

#### 8.安装编译好的模块

【**终端**】键入**sudo make modules_install** 命令，完成模块的安装操作
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/afa3f50c15dbfff28e0df2e040b17bbf.png#pic_center)

这个命令执行也需要一定的时间，耐心等待就好。

#### 9.安装内核

【**终端**】输入 **sudo make install**
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/09feaeaa2ee0e963913db4e89e15ce7b.png#pic_center)

#### 10.重启系统

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/97a8a06b9315c608e7a5cee04c620034.jpeg#pic_center)
重启过后，内核是自动选择刚编译的新内核，如果不是，则在重启过程中，按下shift键后，手动选择一下即可。

#### 11.可能的错误

**错误1**

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e08dae5c083d6dfa04441ba5e055fcb2.png#pic_center)

从错误信息可以看出，问题出在 debian/canonical-certs.pem 上面。首先，用vim打开内核的配置文件.config，
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1ab236e3c925c9b78e9a42abfeef5ef1.png#pic_center)
然后找到”debian/canonical-certs.pem“所在的位置，将其引号内容置空。
这是原始文件内容。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/7009d2f13e6f08a8a9ea09b3be564d56.png#pic_center)
这是修改后的文件内容。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/75e14f91b39c9ab47a2fe0043e3fcca3.png#pic_center)

------

### 三、实验测试

  这部分是最激动人心的环节，它决定着你的努力，你的时间是否得到了回报，究竟是回报的迟到还是失败的重现？下面就跟着博主来见证一下啦！
  首先，查看一下内核版本是否为新安装的内核版本，然后再去编写程序验证程序的系统调用函数的正确性。

【**终端**】输入 **uname -r** ，查看内核版本

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/e98ce5a4b58808e2684e083fa7bcf93e.jpeg#pic_center)
从返回结果可以看出，已经成功安装了新内核，但系统调用是否有用，还得用程序验证一下：

（1）安装 gcc
在【**终端**】中输入 **apt-get -y install gcc**

（2）编写测试程序 test.c
在【**终端**】中键入 **vim test.c** ,此时新建了一个 test.c 的c文件，在文件输入以下内容：

```c
include<stdio.h>
#include<linux/kernel.h>
#include<sys/syscall.h>
#include<unistd.h>
void main(){
        long a=syscall(439,4);
        printf("the result is %ld.\n",a);
}
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/889524d3e44a7b41eb488bead4e2c998.jpeg#pic_center)
保存后退出，在【**终端**】中依次执行 **gcc test.c -o test.out** 、**./test.out**

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/22184b13f420f38fe128999efb231bb6.jpeg#pic_center)
由 result is 64. ，可以看出，内核编译是成功了！！！

### 四、可能的错误

  在实验过程中，出现了太多的错误，大多是因为没有安装一些相应的依赖包，以及系统调用函数的书写错误而导致，那都是博主的辛酸史，没有必要记录下来，读者们只需要按照上述步骤依次执行，就可以成功完成实验。
  在重启系统时，出现了下图错误（忘记截图了，从网上盗了一张），尝试了挺多的办法，最后从一博文中看到，修改虚拟机内存大小即可解决该问题，博主原先的是2GB，后来修改为4GB，果断解决了问题，成功进入新内核的Ubuntu系统。

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1fbc11a4aa79eeae7d713c813044299c.jpeg#pic_center)
