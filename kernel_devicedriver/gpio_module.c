#include <linux/fs.h>        // Header for file operations
#include <linux/cdev.h>      // Header for character devices
#include <linux/module.h>    // Header for kernel modules
#include <linux/io.h>        // Header for memory-mapped I/O
#include <linux/uaccess.h>   // Header for user space access functions

MODULE_LICENSE("GPL");     // License information
MODULE_AUTHOR("YoungJin Suh"); // Author information
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module"); // Description of the module

#define GPIO_MAJOR        200  // Major number for the device
#define GPIO_MINOR        0    // Minor number for the device
#define GPIO_DEVICE       "gpioled" // Device name
#define GPIO_LED         18    // GPIO pin number for LED (Pin 18)

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

// GPIO register structure defining status and control registers
typedef struct {
  uint32_t status;  // GPIO status register
  uint32_t ctrl;    // GPIO control register
} GPIOregs;

// Macro to access GPIO registers
#define GPIO ((GPIOregs*)GPIOBase)

// RIO (Read Input/Output) register structure for GPIO control
typedef struct {
  uint32_t Out;     // Output register
  uint32_t OE;      // Output enable register
  uint32_t In;      // Input register
  uint32_t InSync;  // Synchronized input register
} rioregs;

static volatile uint32_t *gpio; // Pointer to GPIO base address
static volatile uint32_t *PERIBase; // Pointer to peripheral base address

// Base addresses for GPIO and RIO
static volatile uint32_t *GPIOBase;
static volatile uint32_t *RIOBase;
static uint32_t pin = GPIO_LED; // Use GPIO pin 18

// Macros for accessing RIO registers
#define rio ((rioregs *)RIOBase)  // RIO base register access
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4)) // XOR register block
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4)) // Register block for setting GPIO pins
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4)) // Register block for clearing GPIO pins

// Module initialization function
int init_module(void)
{
  dev_t devno;                // Device number
  unsigned int count;         // Count for character devices
  static void *map;           // Pointer for memory mapping
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

  // Map GPIO memory to virtual address space
  map = ioremap(0x1f00000000, 64 * 1024 * 1024);
  if(!map) { // Check for mapping errors
    printk("Error : mapping GPIO memory\n");
    iounmap(map); // Unmap if there was an error
    return -EBUSY; // Return busy error
  }

  gpio = (volatile uint32_t *)map; // Assign mapped address to gpio pointer

  // Set peripheral base address
  PERIBase = gpio;

  // Set GPIO and RIO base addresses
  GPIOBase = PERIBase + 0xD0000 / 4; // GPIO base address offset
  RIOBase = PERIBase + 0xe0000 / 4;  // RIO base address offset
  volatile uint32_t *PADBase = PERIBase + 0xf0000 / 4; // PAD base address offset

  // Set pointer for PAD registers
  volatile uint32_t *pad = PADBase + 1;   
  
  // Configure the pad settings for the GPIO pin (set to 0x10)
  pad[pin] = 0x10; // Setting the pad configuration for pin 18
    
  // Set GPIO pin to output mode
  rioSET->OE = 0x01 << pin;  // Enable the GPIO pin as output

  // Set initial output state of the GPIO pin to High
  rioSET->Out = 0x01 << pin;  // Set the GPIO pin to logical 1 (High)

  return 0; // Return success
}

// Module cleanup function
void cleanup_module(void)
{
  dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number

  unregister_chrdev_region(devno, 1); // Unregister character device region

  cdev_del(&gpio_cdev); // Delete the character device

  // Unmap GPIO memory if it was mapped
  if (gpio) {
    iounmap(gpio);
  }
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

  // Control GPIO pin number and function setting
  uint32_t fn = 5; // Value to set the GPIO pin's functionality (5 corresponds to a special function)

  // Set the GPIO pin to the specified function
  GPIO[pin].ctrl = fn;

  // Set or clear the GPIO pin based on the user input
  (!strcmp(msg, "0")) ? (rioCLR->Out = 0x01 << pin) : (rioSET->Out = 0x01 << pin); // Control the pin output based on message

  // Log the write operation
  printk("GPIO Device(%d) write : %s(%ld)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, len);

  return count; // Return count of bytes copied
}
