// COP 4600 - HW 4
// Christofer Padilla, Richard Tsai, Matthew Winchester
// output_module.c

#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#define DEVICE_NAME "FIFO_output_module"
#define CLASS_NAME "FIFO_output"
#define BUFF_LEN 2048

extern char * fifo_buffer_ptr;
extern short fifo_buffer_size;
extern short first_byte;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christofer Padilla, Richard Tsai, Matthew Winchester");
MODULE_DESCRIPTION("COP4600 - Programming Assignment 3 - Output_Module");
MODULE_VERSION("2.0");

extern struct mutex queue_mutex;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

static struct file_operations fops = {
  .open = dev_open,
  .read = dev_read,
  .release = dev_release
};

static int majorNum;
static int numberOpens = 0;
static struct class* charClass = NULL;
static struct device* charDevice = NULL;

static char * temp;

static int __init char_init(void)
{
  fifo_buffer_ptr = (char *) vmalloc(sizeof(char) * BUFF_LEN);

  printk(KERN_INFO "FIFODev: Initializing FIFODev\n");

  majorNum = register_chrdev(0, DEVICE_NAME, &fops);

  if(majorNum < 0)
  {
    printk(KERN_ALERT "FIFODev: Failed to register a major number\n");
    return majorNum;
  }

  printk(KERN_INFO "FIFODev: Registered correctly with major number %d\n", majorNum);

  charClass = class_create(THIS_MODULE, CLASS_NAME);
  if(IS_ERR(charClass))
  {
    unregister_chrdev(majorNum, DEVICE_NAME);
    printk(KERN_ALERT "FIFODev: Failed to register device class.\n");
    return PTR_ERR(charClass);
  }

  printk(KERN_INFO "FIFODev: device class registered correctly\n");

  charDevice = device_create(charClass, NULL, MKDEV(majorNum, 0), NULL, DEVICE_NAME);
  if(IS_ERR(charDevice))
  {
    class_destroy(charClass);
    unregister_chrdev(majorNum, DEVICE_NAME);
    printk(KERN_ALERT "FIFODev: Failed to create the device.\n");
    return PTR_ERR(charDevice);
  }

  printk(KERN_INFO "FIFODev: Device class created\n");

  return 0;
}

static void __exit char_exit(void)
{
  device_destroy(charClass, MKDEV(majorNum, 0));
  class_unregister(charClass);
  class_destroy(charClass);
  unregister_chrdev(majorNum, DEVICE_NAME);
  printk(KERN_INFO "FIFODev: Exiting");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
  printk(KERN_INFO "FIFODev: Device has been opened %d time(s)\n", ++numberOpens);
  return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
  printk(KERN_INFO "FIFODev: Device closed\n");
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
  char * to_user;
  int read, err;

  if (len > fifo_buffer_size)
  {
    len = fifo_buffer_size;
  }
 
  to_user = kmalloc(len, GFP_KERNEL);

  mutex_lock(&queue_mutex);

  for (read = 0; read < len; read++)
  {
	  to_user[read] = fifo_buffer_ptr[read];

	  fifo_buffer_size--;
  }

  char *temp = kmalloc(BUFF_LEN, GFP_KERNEL);
  
  memcpy(temp, fifo_buffer_ptr+len, BUFF_LEN-len);

  err = copy_to_user(buffer, to_user, len);

  fifo_buffer_ptr = temp;

  mutex_unlock(&queue_mutex);

  if (err == 0) {
	printk(KERN_INFO "FIFODev: %zu bytes read from FIFO read device.\n", len);

 	return len;
  } else {
	printk(KERN_INFO "FIFODev: Bytes couldn't be read from FIFO read device!\n");

	return -EFAULT;
  }
}


module_init(char_init);
module_exit(char_exit);
