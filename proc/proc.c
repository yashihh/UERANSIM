#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "resident_time.h"

#define PROC_FILENAME "ResidentTime"



struct proc_dir_entry *proc_ue_ResidentTime = NULL;


static int ue_ResidentTime_read(struct seq_file *s, void *v) {
    seq_printf(s, "Current: Resident_time = %d\n", get_Resident_time());
    return 0;
}

static ssize_t proc_ResidentTime_write(struct file *filp, const char __user *buffer,
    size_t len, loff_t *dptr) 
{
    char buf[16];
    unsigned long buf_len = min(len, sizeof(buf) - 1);
    int ResidentTime;

    if (copy_from_user(buf, buffer, buf_len)) {
        printk(KERN_ERR "Failed to read buffer: %s\n", buffer);
        goto err;
    }
    
    buf[buf_len] = 0;
    if (sscanf(buf, "%d", &ResidentTime) != 1) {
        printk(KERN_ERR "Failed to read ResidentTime value: %s\n", buffer);
        goto err;
    }
  
    if (ResidentTime < 0) {
        printk(KERN_ERR "Failed to set ResidentTime value: %d\n", ResidentTime);
        goto err;
    }
    
    set_Resident_time(ResidentTime);
    return strnlen(buf, buf_len);
err:
    return -1;
}

static int proc_ResidentTime_read(struct inode *inode, struct file *file)
{
    return single_open(file, ue_ResidentTime_read, NULL);
}


// int create_proc(void)
// {
//     proc_ue = proc_mkdir("ue", NULL);
//     if (!proc_ue) {
//         printk(KERN_ERR "Failed to create /proc/ue\n");
//     }

//     proc_ue_ResidentTime = proc_create("ResidentTime", (S_IFREG | S_IRUGO | S_IWUGO),
//         proc_ue, &proc_ue_ResidentTime_ops);
//     if (!proc_ue_ResidentTime) {
//         printk(KERN_ERR "Failed to create /proc/ue/ResidentTime\n");
//         goto remove_ResidentTime_proc;
//     }
//     return 0;

//     remove_ResidentTime_proc:
//         remove_proc_entry("ResidentTime", proc_ue);
//     remove_ue_proc:
//         remove_proc_entry("ue", NULL);
//     return -1;
// }

// void remove_proc()
// {
//     remove_proc_entry("ResidentTime", proc_ue);
//     remove_proc_entry("ue", NULL);
// }


// static int hello_proc_show(struct seq_file *m, void *v) {
//     seq_printf(m, "Hello\n");
//     return 0;
// }

// static int hello_proc_open(struct inode *inode, struct file *file) {
//     return single_open(file, hello_proc_show, NULL);
// }

// static const struct file_operations hello_proc_fops = {
//     .owner = THIS_MODULE,
//     .open = hello_proc_open,
//     .read = seq_read,
//     .llseek = seq_lseek,
//     .release = single_release,
// };
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_ue_ResidentTime_ops = {
    .proc_open = proc_ResidentTime_read,
    .proc_read = seq_read,
    .proc_write = proc_ResidentTime_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations proc_ue_ResidentTime_ops = {
    .owner      = THIS_MODULE,
    .open       = proc_ResidentTime_read,
    .read       = seq_read,
    .write      = proc_ResidentTime_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};
#endif

static int __init ResidentTime_init(void) {
    proc_ue_ResidentTime = proc_create(PROC_FILENAME, (S_IFREG | S_IRUGO | S_IWUGO),
        NULL, &proc_ue_ResidentTime_ops);
    if (!proc_ue_ResidentTime) {
        printk(KERN_ERR "Failed to create /proc/ResidentTime\n");
        return -1;
    }
    printk(KERN_INFO "ResidentTime module loaded\n");
    return 0;
}

static void __exit ResidentTime_exit(void) {
    remove_proc_entry(PROC_FILENAME, NULL);
    printk(KERN_INFO "ResidentTime module unloaded\n");
}

module_init(ResidentTime_init);
module_exit(ResidentTime_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yashihh");
