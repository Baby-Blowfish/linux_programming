#include <linux/fs.h>        // File operations header
#include <linux/cdev.h>      // Character device header
#include <linux/module.h>    // Kernel module header
#include <linux/io.h>        // Memory-mapped I/O header
#include <linux/uaccess.h>   // User space access header
#include <linux/gpio.h>      // GPIO access header
#include <linux/interrupt.h> // Interrupt handling header
#include <linux/timer.h>     // Timer functions header
#include <linux/mutex.h>     // Mutex (locking) header
#include <linux/string.h>    // String handling header
#include <linux/sched.h>     // send_sig_info() function header
#include <linux/sched/signal.h> // Signal handling header

MODULE_LICENSE("GPL");     // Declare GPL license for the module
MODULE_AUTHOR("HYOJINKIM"); // Author of the module
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module"); // Description of the module

#define GPIO_MAJOR        200  // Major number for character device
#define GPIO_MINOR        3    // Minor number for character device
#define GPIO_DEVICE       "gpiosignal" // Name of the device file
#define GPIO_LED          589   // GPIO pin number for LED (Pin 18)
#define GPIO_SW           594   // GPIO pin number for switch (Pin 23)

static char msg[BLOCK_SIZE] = {0}; // Buffer for storing messages between user and kernel space

// Function prototypes for device operations
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

// File operations structure linking callbacks to respective functions
static struct file_operations gpio_fops = {
  .owner = THIS_MODULE,      // Pointer to this module
  .read  = gpio_read,        // Read callback function
  .write = gpio_write,       // Write callback function
  .open  = gpio_open,        // Open callback function
  .release = gpio_close,     // Close callback function
};

struct cdev gpio_cdev;       // Character device structure
static int switch_irq;       // IRQ number for switch
static struct timer_list timer; // Timer structure for periodic operations
static struct task_struct *task; // Pointer to user task
static DEFINE_MUTEX(led_mutex); // Mutex for synchronizing LED access

// Interrupt service routine for handling switch press
static irqreturn_t isr_func(int irq, void *data)
{
    if(mutex_trylock(&led_mutex) != 0) // Try to acquire mutex lock for LED
    {
        if(irq == switch_irq && !gpio_get_value(GPIO_LED)) // If interrupt from switch and LED is off
		{
			gpio_set_value(GPIO_LED,1); // Turn on the LED
		}
		else if(irq == switch_irq && gpio_get_value(GPIO_LED)) // If interrupt and LED is on
		{
			// Prepare and send a signal (SIGIO) to user space when LED turns off
			static struct kernel_siginfo sinfo;
			memset(&sinfo, 0, sizeof(struct	kernel_siginfo)); // Initialize signal info structure
			sinfo.si_signo = SIGIO; // Set signal number to SIGIO
			sinfo.si_code = SI_USER; // Signal sent by user
			send_sig_info(SIGIO, &sinfo, task); // Send signal to task (user process)
			gpio_set_value(GPIO_LED, 0); // Turn off the LED
		}
		mutex_unlock(&led_mutex); // Release the LED mutex
    }

    return IRQ_HANDLED; // Return that the interrupt was successfully handled
}

// Timer function to toggle the LED periodically
static void timer_func(struct timer_list *t)
{
    if(mutex_trylock(&led_mutex) != 0) // Try to acquire mutex lock for LED
    {
        static int ledflag = 1; // LED state flag
        gpio_set_value(GPIO_LED, ledflag); // Set LED value based on flag
        ledflag = !ledflag; // Toggle LED state
        mutex_unlock(&led_mutex); // Release mutex
    }

    mod_timer(&timer, jiffies + (1*HZ)); // Re-activate the timer after 1 second
}

// Module initialization function
int init_module(void)
{
    dev_t devno;                // Device number for character device
    unsigned int count;         // Counter for devices
    int err;                    // Error handling variable

    printk(KERN_INFO "Hello module!\n"); // Log message when module is initialized

    mutex_init(&led_mutex); // Initialize mutex for LED control

    try_module_get(THIS_MODULE); // Increment module usage count

    devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number with major and minor numbers

    register_chrdev_region(devno, 1, GPIO_DEVICE); // Register character device region

    cdev_init(&gpio_cdev, &gpio_fops); // Initialize character device with file operations

    gpio_cdev.owner = THIS_MODULE; // Set ownership to this module

    count = -1; // Set unlimited count for devices

    err = cdev_add(&gpio_cdev, devno, count); // Add character device to the system
    if(err < 0) {
        printk("Error : Device Add\n"); // Log error if device addition fails
        return -1;
    }

    printk(" 'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR); // Log mknod command for creating device file
    printk(" 'chmod 666 /dev/%s'\n", GPIO_DEVICE); // Log chmod command for setting permissions

    gpio_request(GPIO_LED, "LED"); // Request GPIO pin for LED

    gpio_direction_output(GPIO_LED, 0); // Set GPIO direction to output and turn LED off initially

    gpio_request(GPIO_SW, "SWITCH"); // Request GPIO pin for switch
    gpio_direction_input(GPIO_SW); // Set GPIO direction to input for switch

    switch_irq = gpio_to_irq(GPIO_SW); // Get IRQ number for the switch

    err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch", NULL); // Request interrupt for switch
    return 0; // Return 0 if successful
}

// Module cleanup function
void cleanup_module(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // Create device number

    mutex_destroy(&led_mutex); // Destroy mutex

    unregister_chrdev_region(devno, 1); // Unregister character device region

    cdev_del(&gpio_cdev); // Delete character device

    free_irq(switch_irq, NULL); // Free IRQ for switch

    gpio_free(GPIO_LED); // Free GPIO pin for LED
    gpio_free(GPIO_SW);  // Free GPIO pin for switch

    module_put(THIS_MODULE); // Decrement module usage count

    printk(KERN_INFO "Good-bye module!\n"); // Log message when module is cleaned up
}

// Open function called when device is opened
static int gpio_open(struct inode *inode, struct file *fil)
{
    printk("GPIO Device opened(%d/%d)\n", imajor(inode), iminor(inode)); // Log open event with major and minor numbers
    return 0; // Return success
}

// Close function called when device is closed
static int gpio_close(struct inode *inode, struct file *fil)
{
    printk("GPIO Device closed(%d)\n", MAJOR(fil->f_path.dentry->d_inode->i_rdev)); // Log close event
    return 0; // Return success
}

// Read function called when data is read from device
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
    int count; // Variable for number of bytes copied to user space

    strcat(msg, " from Kernel"); // Append message to buffer
    count = copy_to_user(buff, msg, strlen(msg) + 1); // Copy buffer to user space

    printk("GPIO Device(%d) read : %s(%d)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, count); // Log read operation

    return count; // Return number of bytes read
}

// Write function called when data is written to device
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count; // Variable for number of bytes copied from user space
	char *cmd, *str;
	char *sep = ":";
	char *endptr, *pidstr;
	pid_t pid;

    memset(msg, 0, BLOCK_SIZE); // Clear message buffer

    count = copy_from_user(msg, buff, len); // Copy data from user space to kernel space

	str = kstrdup(msg, GFP_KERNEL); // Duplicate string for manipulation
	cmd = strsep(&str, sep); // Extract command from message
	pidstr = strsep(&str, sep); // Extract PID from message
	printk("Command : %s, Pid : %s\n", cmd, pidstr); // Log command and PID
	cmd[1] = '\0'; // Null-terminate command

    if(!strcmp(cmd, "0")) // If command is "0"
    {
        del_timer_sync(&timer); // Delete the timer
        gpio_set_value(GPIO_LED, 0); // Turn off the LED
    }
    else // If command is not "0"
    {
		if(kstrtol(pidstr, 10, &endptr)) { // Convert PID string to long (PID)
			task = pid_task(find_vpid(pid), PIDTYPE_PID); // Find task corresponding to PID
			if(task == NULL) // If task not found
				printk("Error : Invalid pid\n"); // Log error
		}
        mod_timer(&timer, jiffies + (1*HZ)); // Modify the timer to run after 1 second
    }

    printk("GPIO Device(%d) write : %s(%d)\n", MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, count); // Log write operation

    return count; // Return number of bytes written
}
