#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/errno.h>

#include "msg.h"

MODULE_LICENSE("GPL");

int isEnableBlockFile = 0;
int isEnableProtectKill = 0;
void ** sctable ;

char bf_filename[FNAME_SIZE] = { 0x0, } ;
uid_t bf_uid;
uid_t pk_uid;

asmlinkage int (*orig_sys_open)(const char __user * filename, int flags, umode_t mode);
asmlinkage int (*orig_sys_kill)(pid_t pid, int sig);

asmlinkage int mousehole_sys_open(const char __user * filename, int flags, umode_t mode) {
	if (!isEnableBlockFile) return orig_sys_open(filename, flags, mode);

	char fname[FNAME_SIZE] = {0x0};

	copy_from_user(fname, filename, FNAME_SIZE) ;

	//printk(KERN_INFO "mousehole: hook open : %s\n", fname);
	if (strstr(fname, bf_filename) != NULL && current->cred->uid.val == bf_uid) {
		printk(KERN_INFO "mousehole: Block file open - %s\n", fname);
		return -EACCES;
	}
	return orig_sys_open(filename, flags, mode) ;
}

asmlinkage int mousehole_sys_kill(pid_t pid, int sig) {
	if (!isEnableProtectKill) return orig_sys_kill(pid, sig);

	//printk(KERN_INFO "mousehole: mousehole_sys_kill hook orig_sys_kill\n");
	struct task_struct* t;

	for_each_process(t) {
		if (t->pid == pid && t->cred->uid.val == pk_uid) {
			printk(KERN_INFO "mousehole: Protect kill - %s:%d\n", t->comm, t->cred->uid.val);
			return -EACCES;
		}
	}

	return orig_sys_kill(pid, sig);
}

static 
int mousehole_proc_open(struct inode *inode, struct file *file) {
	return 0 ;
}

static 
int mousehole_proc_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t mousehole_proc_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) {
	char buf[256] ;
	ssize_t toread ;

	sprintf(buf, "mousehole status\nBlock file open: %s", isEnableBlockFile==1?"Enable":"Disable") ;

	if (strlen(buf) >= *offset + size) {
		toread = size ;
	}
	else {
		toread = strlen(buf) - *offset ;
	}

	if (copy_to_user(ubuf, buf + *offset, toread))
		return -EFAULT ;	

	*offset = *offset + toread ;

	return toread ;
}

static 
ssize_t mousehole_proc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[MSG_SIZE] = {0x0};

	if (*offset != 0 || size > MSG_SIZE)
		return -EFAULT;

	if (copy_from_user(buf, ubuf, size))
		return -EFAULT;

	
	int type = 0;

	sscanf(buf, "%d", &type); 
	printk(KERN_INFO "mousehole: Msg type is: %d\n", type);

	switch (type) {
		case BLOCK_FILE:
			sscanf(buf, "%d %d %s", &type, &bf_uid, bf_filename);
			isEnableBlockFile = 1;
			break;
		case PREVENT_KILL:
			sscanf(buf, "%d %d", &type, &pk_uid);
			isEnableProtectKill = 1;
			break;
		case UNDO_FILE:
			bf_uid = 0;
			bf_filename[0] = '\0';
			isEnableBlockFile = 0;
			break;
    	case UNDO_KILL:
			pk_uid = 0;
			isEnableProtectKill = 0;
			break;
	}

	printk(KERN_INFO "mousehole: received msg: %s\n", buf);

	*offset = strlen(buf);

	return *offset;
}

static const struct file_operations mousehole_fops = {
	.owner = 	THIS_MODULE,
	.open = 	mousehole_proc_open,
	.read = 	mousehole_proc_read,
	.write = 	mousehole_proc_write,
	.llseek = 	seq_lseek,
	.release = 	mousehole_proc_release,
} ;

static 
int __init mousehole_init(void) {
	unsigned int level ; 
	pte_t * pte ;

	proc_create("mousehole", S_IRUGO | S_IWUGO, NULL, &mousehole_fops) ;

	sctable = (void *) kallsyms_lookup_name("sys_call_table") ;

	orig_sys_open = sctable[__NR_open] ;
	orig_sys_kill = sctable[__NR_kill] ;

	pte = lookup_address((unsigned long) sctable, &level) ;
	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW ;	

	sctable[__NR_open] = mousehole_sys_open;
	sctable[__NR_kill] = mousehole_sys_kill;

	return 0;
}

static 
void __exit mousehole_exit(void) {
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("mousehole", NULL) ;

	sctable[__NR_open] = orig_sys_open;
	sctable[__NR_kill] = orig_sys_kill;
	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(mousehole_init);
module_exit(mousehole_exit);
