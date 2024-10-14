#include <linux/fs.h>        // Header for file operations
#include <linux/cdev.h>      // Header for character devices
#include <linux/module.h>    // Header for kernel modules
#include <linux/io.h>        // Header for memory-mapped I/O
#include <linux/uaccess.h>   // Header for user space access functions
#include <linux/gpio.h>      // Header for GPIO access
#include <linux/interrupt.h> // Header for interrupt handling
#include <linux/timer.h>     // Header for timer functions
#include <linux/mutex.h>     // Header for mutex (locking mechanism)

MODULE_LICENSE("GPL");     // License information for the kernel module
MODULE_AUTHOR("HYOJINKIM"); // Author information for this kernel module
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module"); // Description of the kernel module

#define GPIO_MAJOR        200  // Major number for the character device
#define GPIO_MINOR        2    // Minor number for the character device
#define GPIO_DEVICE       "gpiotimer" // Device name used in device files
#define GPIO_LED          589   // GPIO pin number for the LED (Pin 18)
#define GPIO_SW           594   // GPIO pin number for the switch (Pin 23)

static char msg[BLOCK_SIZE] = {0}; // Buffer for messages between user and kernel space

// Function prototypes for file operations
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

// File operations structure with callbacks
static struct file_operations gpio_fops = {
  .owner = THIS_MODULE,      // Module ownership
  .read  = gpio_read,        // Read operation callback
  .write = gpio_write,       // Write operation callback
  .open  = gpio_open,        // Open operation callback
  .release = gpio_close,     // Release (close) operation callback
};

struct cdev gpio_cdev;       // Character device structure for GPIO handling
static int switch_irq;       // IRQ number for switch
static struct timer_list timer; // Timer structure for periodic operations
static DEFINE_MUTEX(led_mutex); // Mutex for synchronizing access to LED

// Interrupt Service Routine (ISR) for handling switch press interrupts
static irqreturn_t isr_func(int irq, void *data)
{
    if(mutex_trylock(&led_mutex) != 0) // Try to acquire the LED mutex
    {
        if((irq == switch_irq) && (!gpio_get_value(GPIO_LED))) // If the switch is pressed and LED is off
        {
            gpio_set_value(GPIO_LED, 1); // Turn on the LED
        }
        else if(irq == switch_irq && gpio_get_value(GPIO_LED)) // If the switch is pressed and LED is on
        {
            gpio_set_value(GPIO_LED, 0); // Turn off the LED
        }
        mutex_unlock(&led_mutex); // Release the LED mutex
    }

    return IRQ_HANDLED; // Return to indicate interrupt has been handled
}

// Timer function to toggle the LED periodically
static void timer_func(struct timer_list *t)
{
    if(mutex_trylock(&led_mutex) != 0) // Try to acquire the LED mutex
    {
        static int ledflag = 1; // Flag to track LED state
        gpio_set_value(GPIO_LED, ledflag); // Set LED value according to flag
        ledflag = !ledflag; // Toggle the flag
        mutex_unlock(&led_mutex); // Release the mutex
    }

    mod_timer(&timer, jiffies + (1*HZ)); // Schedule the timer to run again after 1 second
}

// Module initialization function
int init_module(void)
{
    dev_t devno;                // Device number
    unsigned int count;         // Counter for character devices
    int err;                    // Error code variable

    printk(KERN_INFO "Hello module!\n"); // Log message when the module is initialized

    mutex_init(&led_mutex); // Initialize the mutex for LED control

    try_module_get(THIS_MODULE); // Increment the module usage count

    devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create a device number

    register_chrdev_region(devno, 1, GPIO_DEVICE); // Register character device region

    cdev_init(&gpio_cdev, &gpio_fops); // Initialize the character device with file operations

    gpio_cdev.owner = THIS_MODULE; // Set ownership of the device to this module

    count = -1; // Set count to -1 (unlimited devices)

    err = cdev_add(&gpio_cdev, devno, count); // Add character device to the system
    if(err < 0) {
        printk("Error : Device Add\n"); // Log error if the device addition fails
        return -1;
    }

    printk(" 'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR); // Log message for mknod command
    printk(" 'chmod 666 /dev/%s'\n", GPIO_DEVICE); // Log message for chmod command

    gpio_request(GPIO_LED, "LED"); // Request GPIO pin for LED

    gpio_direction_output(GPIO_LED, 0); // Set GPIO pin as output for LED and turn it off initially

    gpio_request(GPIO_SW, "SWITCH"); // Request GPIO pin for switch
    gpio_direction_input(GPIO_SW); // Set GPIO pin as input for switch

    switch_irq = gpio_to_irq(GPIO_SW); // Get the IRQ number for the switch

    err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch", NULL); // Request IRQ for switch press
    return 0;
}

// Module cleanup function
void cleanup_module(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number

    mutex_destroy(&led_mutex); // Destroy the mutex

    unregister_chrdev_region(devno, 1); // Unregister the character device region

    cdev_del(&gpio_cdev); // Delete the character device

    free_irq(switch_irq, NULL); // Free the IRQ associated with the switch

    gpio_free(GPIO_LED); // Free the GPIO pin for LED
    gpio_free(GPIO_SW);  // Free the GPIO pin for switch

    module_put(THIS_MODULE); // Decrement the module usage count

    printk(KERN_INFO "Good-bye module!\n"); // Log message when the module is cleaned up
}

// Open function for the GPIO device
static int gpio_open(struct inode *inode, struct file *fil)
{
    printk("GPIO Device opened(%d/%d)\n", imajor(inode), iminor(inode)); // Log device open
    return 0;
}

// Close function for the GPIO device
static int gpio_close(struct inode *inode, struct file *fil)
{
    printk("GPIO Device closed(%d)\n", MAJOR(fil->f_path.dentry->d_inode->i_rdev)); // Log device close
    return 0;
}

// Read function for the GPIO device
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
    int count; // Variable to hold the number of bytes copied to user space

    strcat(msg, " from Kernel"); // Append kernel message to buffer
    count = copy_to_user(buff, msg, strlen(msg) + 1); // Copy buffer to user space

    printk("GPIO Device(%d) read : %s(%d)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, count); // Log read operation

    return count;
}

// Write function for the GPIO device
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count; // Variable to hold the number of bytes copied from user space

    memset(msg, 0, BLOCK_SIZE); // Clear the message buffer

    count = copy_from_user(msg, buff, len); // Copy data from user space to kernel buffer

    if(!strcmp(msg, "0")) // If the message is "0"
    {
        del_timer_sync(&timer); // Delete the timer
        gpio_set_value(GPIO_LED, 0); // Turn off the LED
    }
    else // If the message is not "0"
    {
        timer_setup(&timer, timer_func, 0); // Set up the timer
        timer.expires = jiffies + (1*HZ); // Set timer to expire after 1 second
        add_timer(&timer); // Add the timer to the system
    }

    printk("GPIO Device(%d) write : %s(%ld)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, len); // Log write operation

    return count;
}
