#include <linux/fs.h>        // Header for file operations
#include <linux/cdev.h>      // Header for character devices
#include <linux/module.h>    // Header for kernel modules
#include <linux/io.h>        // Header for memory-mapped I/O
#include <linux/uaccess.h>   // Header for user space access functions
#include <linux/gpio.h>

MODULE_LICENSE("GPL");     // License information
MODULE_AUTHOR("HYOJINKIM"); // Author information
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module"); // Description of the module

#define GPIO_MAJOR        200  // Major number for the device
#define GPIO_MINOR        0    // Minor number for the device
#define GPIO_DEVICE       "gpioled" // Device name
#define GPIO_LED		  589    // GPIO pin number for LED (Pin 18)   "cat /sys/kernel/debug/gpio"

// Buffer for messages exchanged between user space and kernel space
static char msg[BLOCK_SIZE] = {0};

// Function prototypes for file operations
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

// File operations structure
static struct file_operations gpio_fops = {
  .owner = THIS_MODULE,      // This module owns the file operations
  .read  = gpio_read,        // Read operation
  .write = gpio_write,       // Write operation
  .open  = gpio_open,        // Open operation
  .release = gpio_close,     // Close operation
};

struct cdev gpio_cdev;       // Character device structure


// Module initialization function
int init_module(void)
{
  dev_t devno;                // Device number
  unsigned int count;         // Count for character devices
  int err;                    // Error code

  printk(KERN_INFO "Hello module!\n"); // Log module initialization

  try_module_get(THIS_MODULE); // Increment module usage count

  devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number

  // Register character device region
  register_chrdev_region(devno, 1, GPIO_DEVICE);

  // Initialize character device structure with file operations
  cdev_init(&gpio_cdev, &gpio_fops);

  gpio_cdev.owner = THIS_MODULE; // Set owner of the character device

  count = -1; // Count is set to -1 for unlimited devices

  // Add character device to the system
  err = cdev_add(&gpio_cdev, devno, count);
  if(err < 0) { // Check for errors
    printk("Error : Device Add\n");
    return -1; // Return error if adding device fails
  }

  printk(" 'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR); // Log mknod command
  printk(" 'chmod 666 /dev/%s'\n", GPIO_DEVICE); // Log chmod command

  gpio_request(GPIO_LED, "LED");
  gpio_direction_output(GPIO_LED, 0);


  return 0; // Return success
}

// Module cleanup function
void cleanup_module(void)
{
  dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number

  unregister_chrdev_region(devno, 1); // Unregister character device region

  cdev_del(&gpio_cdev); // Delete the character device
	
  gpio_free(GPIO_LED);
  gpio_direction_output(GPIO_LED, 0);

  module_put(THIS_MODULE); // Decrement module usage count
  
  printk(KERN_INFO "Good-bye module!\n"); // Log module cleanup
}

// Open function for the GPIO device
static int gpio_open(struct inode *inode, struct file *fil)
{
  printk("GPIO Device opened(%d/%d)\n", imajor(inode), iminor(inode)); // Log device opening
  return 0; // Return success
}

// Close function for the GPIO device
static int gpio_close(struct inode *inode, struct file *fil)
{
  printk("GPIO Device closed(%d)\n", MAJOR(fil->f_path.dentry->d_inode->i_rdev)); // Log device closing
  return 0; // Return success
}

// Read function for the GPIO device
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
  int count; // Variable to hold copy count

  strcat(msg, " from Kernel"); // Append string to message
  count = copy_to_user(buff, msg, strlen(msg) + 1); // Copy message to user buffer

  // Log the read operation
  printk("GPIO Device(%d) read : %s(%d)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, count);

  return count; // Return count of bytes copied
}

// Write function for the GPIO device
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
  short count; // Variable to hold copy count

  memset(msg, 0, BLOCK_SIZE); // Clear the message buffer

  count = copy_from_user(msg, buff, len); // Copy message from user buffer

  gpio_set_value(GPIO_LED,(!strcmp(msg, "0"))?0:1);

  // Log the write operation
  printk("GPIO Device(%d) write : %s(%ld)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, len);

  return count; // Return count of bytes copied
}
