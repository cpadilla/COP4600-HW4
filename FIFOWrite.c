// COP 4600 - HW 3
// Christopher Padilla, Richard Tsai, Matthew Winchester
// input_module.c

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#define DEVICE_NAME "FIFO_input_module"
#define CLASS_NAME "FIFO_input"
#define BUFF_LEN 2048

static char * fifo_buffer_ptr;
EXPORT_SYMBOL(fifo_buffer_ptr);
static short fifo_buffer_size;
EXPORT_SYMBOL(fifo_buffer_size);
static short first_byte;
EXPORT_SYMBOL(first_byte);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christofer Padilla, Richard Tsai, Matthew Winchester");
MODULE_DESCRIPTION("COP4600 - Programming Assignment 3 - Input_Module");
MODULE_VERSION("2.0"); 
struct mutex queue_mutex;
EXPORT_SYMBOL(queue_mutex);

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
  .open = dev_open,
  .write = dev_write,
  .release = dev_release
};

static int majorNum;
static int numberOpens = 0;
static struct class* charClass = NULL;
static struct device* charDevice = NULL;

static int __init char_init(void)
{
  fifo_buffer_ptr = (char *) vmalloc(sizeof(char) * BUFF_LEN);

  printk(KERN_INFO "FIFODev: Initializing FIFODev\n");

  majorNum = register_chrdev(0, DEVICE_NAME, &fops);

  mutex_init(&queue_mutex);

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
  mutex_destroy(&queue_mutex);
  printk(KERN_INFO "FIFODev: Exiting");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
  // if(!mutex_trylock(&input_mutex))
  // {
  //   printk(KERN_INFO "FIFODev: Device in use by another process");
  //   return -EBUSY;
  // }
  printk(KERN_INFO "FIFODev: Device has been opened %d time(s)\n", ++numberOpens);
  return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
  printk(KERN_INFO "FIFODev: Device closed\n");
  return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    mutex_lock(&queue_mutex);

    printk(KERN_INFO "FIFODev: Current fifo_buffer_size: %d\n",fifo_buffer_size);
    printk(KERN_INFO "FIFODev: Received %zu characters from the user\n", len);

    int i = 0;
    for(; fifo_buffer_size < BUFF_LEN && i < len; fifo_buffer_size++) {
        if ((first_byte + fifo_buffer_size) < BUFF_LEN) {
	        fifo_buffer_ptr[(first_byte+fifo_buffer_size)] = buffer[i++];
        }
    }

    printk(KERN_INFO "FIFODev: After copy fifo_buffer_size: %d\n",fifo_buffer_size);
    mutex_unlock(&queue_mutex);

    return len;

}

module_init(char_init);
module_exit(char_exit);
