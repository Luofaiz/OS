#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
extern unsigned long volatile pfcount;
static int my_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Pfcount is %ld !\n", pfcount);
    return 0;
}
static int my_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_proc_show, NULL);
}

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
    struct proc_dir_entry *parent = proc_mkdir("os4",NULL);
    file = proc_create("pfcount", 0644, parent, &my_fops);
    if(!file)
        return -ENOMEM;
    return 0;
}
 
static void __exit my_exit(void)
{
  remove_proc_entry("os4", NULL);
}

module_init(my_init);
module_exit(my_exit);

