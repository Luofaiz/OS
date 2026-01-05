#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/jiffies.h>

extern unsigned long volatile pfcount;

// 统计数据结构
static struct {
    unsigned long start_time;
    unsigned long initial_pfcount;
    unsigned long max_pfcount;
    unsigned long last_pfcount;
    unsigned long last_check_time;
    unsigned long reset_count;  // 重置次数
} stats;

// 显示统计信息
static int pfcount_show(struct seq_file *m, void *v)
{
    unsigned long current_time = jiffies;
    unsigned long runtime_sec = (current_time - stats.start_time) / HZ;
    unsigned long current_pfcount = pfcount;
    unsigned long total_pf = current_pfcount - stats.initial_pfcount;
    unsigned long pf_rate = runtime_sec > 0 ? total_pf / runtime_sec : 0;
    
    // 更新最大值
    if (current_pfcount > stats.max_pfcount) {
        stats.max_pfcount = current_pfcount;
    }
    
    seq_printf(m, "=== OS4 缺页监控系统 v1.1 ===\n");
    seq_printf(m, "模块运行时间: %lu秒 (%lu分%lu秒)\n", 
               runtime_sec, runtime_sec/60, runtime_sec%60);
    seq_printf(m, "统计重置次数: %lu\n", stats.reset_count);
    seq_printf(m, "\n[缺页统计]\n");
    seq_printf(m, "当前总缺页数: %lu\n", current_pfcount);
    seq_printf(m, "监控期间缺页: %lu\n", total_pf);
    seq_printf(m, "历史峰值: %lu\n", stats.max_pfcount);
    seq_printf(m, "平均缺页率: %lu/秒\n", pf_rate);
    
    // 缺页率分级显示
    seq_printf(m, "\n[性能评级]\n");
    if (pf_rate == 0) {
        seq_printf(m, "系统空闲\n");
    } else if (pf_rate < 5) {
        seq_printf(m, "性能优秀 (低负载)\n");
    } else if (pf_rate < 20) {
        seq_printf(m, "性能良好 (中负载)\n");
    } else if (pf_rate < 50) {
        seq_printf(m, "系统繁忙 (高负载)\n");
    } else {
        seq_printf(m, "系统高压 (超高负载)\n");
    }
    
    stats.last_pfcount = current_pfcount;
    stats.last_check_time = current_time;
    
    return 0;
}

// 配置文件显示和写入
static int config_show(struct seq_file *m, void *v)
{
    seq_printf(m, "=== 配置选项 ===\n");
    seq_printf(m, "可用命令:\n");
    seq_printf(m, "  reset     - 重置统计数据\n");
    seq_printf(m, "  status    - 显示当前状态\n");
    seq_printf(m, "  info      - 显示模块信息\n");
    seq_printf(m, "\n使用方法:\n");
    seq_printf(m, "  echo 'reset' > /proc/os4/config\n");
    seq_printf(m, "\n当前状态: 运行中\n");
    seq_printf(m, "重置次数: %lu\n", stats.reset_count);
    
    return 0;
}

static ssize_t config_write(struct file *file, const char __user *buffer, 
                           size_t count, loff_t *ppos)
{
    char cmd[32];
    
    if (count >= sizeof(cmd))
        return -EINVAL;
        
    if (copy_from_user(cmd, buffer, count))
        return -EFAULT;
        
    cmd[count] = '\0';
    
    // 移除换行符
    if (cmd[count-1] == '\n')
        cmd[count-1] = '\0';
    
    if (strcmp(cmd, "reset") == 0) {
        stats.start_time = jiffies;
        stats.initial_pfcount = pfcount;
        stats.max_pfcount = pfcount;
        stats.last_pfcount = pfcount;
        stats.last_check_time = jiffies;
        stats.reset_count++;
        
        printk(KERN_INFO "OS4: 统计已重置，重置次数: %lu\n", stats.reset_count);
        
    } else if (strcmp(cmd, "status") == 0) {
        printk(KERN_INFO "OS4: 当前缺页数=%lu, 运行时间=%lu秒\n", 
               pfcount, (jiffies - stats.start_time) / HZ);
               
    } else if (strcmp(cmd, "info") == 0) {
        printk(KERN_INFO "OS4: 缺页监控模块 v1.1\n");
    } else {
        printk(KERN_WARNING "OS4: 未知命令: %s\n", cmd);
        return -EINVAL;
    }
    
    return count;
}

// 文件操作结构
static int pfcount_open(struct inode *inode, struct file *file)
{
    return single_open(file, pfcount_show, NULL);
}

static int config_open(struct inode *inode, struct file *file)
{
    return single_open(file, config_show, NULL);
}

static struct file_operations pfcount_fops = {
    .owner   = THIS_MODULE,
    .open    = pfcount_open,
    .release = single_release,
    .read    = seq_read,
    .llseek  = seq_lseek,
};

static struct file_operations config_fops = {
    .owner   = THIS_MODULE,
    .open    = config_open,
    .read    = seq_read,
    .write   = config_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int __init my_init(void)
{
    struct proc_dir_entry *parent;
    struct proc_dir_entry *pfcount_file;
    struct proc_dir_entry *config_file;
    
    // 创建proc目录
    parent = proc_mkdir("os4", NULL);
    if (!parent)
        return -ENOMEM;
    
    // 创建统计文件
    pfcount_file = proc_create("pfcount", 0644, parent, &pfcount_fops);
    if (!pfcount_file) {
        remove_proc_entry("os4", NULL);
        return -ENOMEM;
    }
    
    // 创建配置文件
    config_file = proc_create("config", 0644, parent, &config_fops);
    if (!config_file) {
        remove_proc_entry("pfcount", parent);
        remove_proc_entry("os4", NULL);
        return -ENOMEM;
    }
    
    // 初始化统计数据
    stats.start_time = jiffies;
    stats.initial_pfcount = pfcount;
    stats.max_pfcount = pfcount;
    stats.last_pfcount = pfcount;
    stats.last_check_time = jiffies;
    stats.reset_count = 0;
    
    printk(KERN_INFO "OS4: 缺页监控模块已加载\n");
    printk(KERN_INFO "     统计文件: /proc/os4/pfcount\n");
    printk(KERN_INFO "     配置文件: /proc/os4/config\n");
    printk(KERN_INFO "     初始缺页数: %lu\n", stats.initial_pfcount);
    
    return 0;
}

static void __exit my_exit(void)
{
    unsigned long final_pfcount = pfcount;
    unsigned long total_runtime = (jiffies - stats.start_time) / HZ;
    
    printk(KERN_INFO "OS4: 模块卸载\n");
    printk(KERN_INFO "     运行时间: %lu秒\n", total_runtime);
    printk(KERN_INFO "     缺页增长: %lu\n", final_pfcount - stats.initial_pfcount);
    printk(KERN_INFO "     重置次数: %lu\n", stats.reset_count);
    
    remove_proc_entry("os4", NULL);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("OS4 Page Fault Monitor with Config v1.1");
