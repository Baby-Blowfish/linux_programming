#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HYOJINKIM");
MODULE_DESCRIPTION("RaspberryPi GPIO LED Device Module");

#define GPIO_MAJOR		200
#define GPIO_MINOR		0
#define GPIO_DEVICE		"gpioled"
#define GPIO_LED		18

volatile unsigned *gpio;
static char msg[BLOCK_SIZE] = {0};

static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_read(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

static struct file_operations gpio_fops = {
	.owner = THIS_MODULE;
	.read  = gpio_read;
	.write = gpio_write;
	.open  = gpio_open;
	.release = gpio_close,
}

struct cdev gpio_cdev;

// GPIO 레지스터를 표현하는 구조체. GPIO의 상태와 제어를 담당하는 레지스터 포함.
typedef struct {
    uint32_t status;  // GPIO 상태 레지스터
    uint32_t ctrl;    // GPIO 제어 레지스터
} GPIOregs;

// GPIO 레지스터를 쉽게 접근하기 위해 정의한 매크로
#define GPIO ((GPIOregs*)GPIOBase)

// RIO(입출력) 레지스터를 위한 구조체 정의. GPIO 핀의 입출력 제어와 관련된 레지스터 포함.
typedef struct {
    uint32_t Out;     // 출력 레지스터
    uint32_t OE;      // 출력 enable 레지스터
    uint32_t In;      // 입력 레지스터
    uint32_t InSync;  // 동기화된 입력 레지스터
} rioregs;

// RIO 레지스터에 접근하기 위한 매크로 정의
#define rio ((rioregs *)RIOBase)
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))  // XOR 연산을 위한 레지스터 블록
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))  // GPIO 핀을 설정하는 레지스터 블록
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))  // GPIO 핀을 클리어하는 레지스터 블록



int init_module(void)
{
	dev_t devno;
	unsigned int count;
	static void *map;
	int err;

	printk(KERN_INFO "Hello module!\n");

	try_module_get(THIS_MDULE);

	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	register_chrdev_region(devno, 1, GPIO_DEVICE);

	cdev_init(&gpio_cdev, &gpio_fops);

	gpio_cdev.owner = THIS_MODULE;
	
	count = -1;
	
	err = cdev_add(&gpio_cdev, devno, count);
	if(err<0)
	{
		printk("Error : Device Add\n");
		return -1;
	}

	printk(" 'mknod /dev/%s c %d 0'\n", GPIO_DEVICE,GPIO_MAJOR);
	printk(" 'chmod 666 /dev/%s'\n", GPIO_DEVICE);
	
	map = ioremap(0x1f00000000,64 * 1024 * 1024);
	if(!map)
	{
		printk("Error : mapping GPIO memory\n");
		iounmap(map);
		return -EBUSY;
	}

	gpio = (volatile unsigned int *)map;

    // PERIBase는 매핑된 메모리의 시작 주소
    uint32_t *PERIBase = gpio;

    // GPIO 및 RIO 베이스 주소 설정
    uint32_t *GPIOBase = PERIBase + 0xD0000 / 4; // GPIO 베이스 주소는 0xD0000 오프셋에 있음
    uint32_t *RIOBase = PERIBase + 0xe0000 / 4;  // RIO 베이스 주소는 0xE0000 오프셋에 있음
    uint32_t *PADBase = PERIBase + 0xf0000 / 4;  // PAD 베이스 주소는 0xF0000 오프셋에 있음

    // PAD 레지스터에 대한 포인터 설정
    uint32_t *pad = PADBase + 1;   
    
    // 제어할 GPIO 핀 번호 및 함수 설정
    uint32_t pin = GPIO_LED;  // GPIO 핀 18 사용
    uint32_t fn = 5;    // 해당 핀의 기능을 설정하는 값 (5는 해당 핀의 특별한 기능을 의미)

    // GPIO 핀을 해당 기능으로 설정
    GPIO[pin].ctrl = fn;

    // 패드 설정. 해당 핀의 패드에 대한 설정 값을 0x10으로 설정
    pad[pin] = 0x10;

    // RIO SET 레지스터에서 출력 enable (OE) 설정
    rioSET->OE = 0x01 << pin;  // 해당 핀을 출력 모드로 설정

    // RIO SET 레지스터에서 출력 (Out) 설정
    rioSET->Out = 0x01 << pin;  // 해당 핀에 논리 1 출력 (High 상태)

	return 0;
}

void cleanup_module(void)
{
	
	dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	unregister_chrdev_region(devno, 1);

	cdev_del(&gpio_cdev);

	if (gpio)
	{
		iounmap(gpio);
	}
	module_put(THIS_MODULE);
	printk(KERN_INFO "Good-bye module!\n");
}


static int gpio_open(struct inode * inode, struct file * *fil)
{
	printk("GPIO DEvice opened(%d/%d)\n",imajor(inod), iminor(inod));
	return 0;
}

tatic int gpio_close(struct inode * inode, struct file *fil)
{
	printk("GPIO DEvice closed(%d)\n",MAJOR(fil->f_path.dentry->d_inod->i_rdev));
	return 0;
}

static ssize_t gpio_read(struct file * inode , const char * buff, size_t len, loff_t * *off)
{
	int count;

	strcat(msg, " from Kernel");
	count = copy_to_user(buff, msg, strlen(msg)+1);
	
	printk("GPIO Device(%d) read : %s(%d)\n",MAJOR(inode->f_path.dentry->d_inod->i_rdev),msg, count);

	return count;
}

static ssize_t gpio_write(struct file * inode , const char * buff, size_t len, loff_t * *off)
{
	short count;

	memset(msg,0,BLOCK_SIZE);	// BLOCK_SIZE

	count = copy_from_user(msg,buff, len);
	

    // PERIBase는 매핑된 메모리의 시작 주소
    uint32_t *PERIBase = gpio;

    // GPIO 및 RIO 베이스 주소 설정
    uint32_t *GPIOBase = PERIBase + 0xD0000 / 4; // GPIO 베이스 주소는 0xD0000 오프셋에 있음
    uint32_t *RIOBase = PERIBase + 0xe0000 / 4;  // RIO 베이스 주소는 0xE0000 오프셋에 있음
    uint32_t *PADBase = PERIBase + 0xf0000 / 4;  // PAD 베이스 주소는 0xF0000 오프셋에 있음

    // PAD 레지스터에 대한 포인터 설정
    uint32_t *pad = PADBase + 1;   
    
    // 제어할 GPIO 핀 번호 및 함수 설정
    uint32_t pin = GPIO_LED;  // GPIO 핀 18 사용
    uint32_t fn = 5;    // 해당 핀의 기능을 설정하는 값 (5는 해당 핀의 특별한 기능을 의미)

    // GPIO 핀을 해당 기능으로 설정
    GPIO[pin].ctrl = fn;

    // 패드 설정. 해당 핀의 패드에 대한 설정 값을 0x10으로 설정
    pad[pin] = 0x10;

    // RIO SET 레지스터에서 출력 enable (OE) 설정
    rioSET->OE = 0x01 << pin;  // 해당 핀을 출력 모드로 설정

    // RIO SET 레지스터에서 출력 (Out) 설정
	(!strcmp(msg,"0"))?(rioSET->Out = 0x01 << pin):(rioCLR->OUT = 0x01 << pin);

	printk("GPIO Device(%d) write : %s(%d)\n",MAJOR(inode->f_path.dentry->d_inod->i_rdev),msg, len);

	return count;
}
