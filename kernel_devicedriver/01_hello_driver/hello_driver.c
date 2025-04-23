/// @file hello_driver.c
/// @brief Hello World 커널 모듈 예제 - 커널 모듈 구조를 익히는 기본 실습
/// @author Kim Hyo Jin

#include <linux/module.h>   // 모듈 정의 매크로 (MODULE_LICENSE 등)
#include <linux/kernel.h>   // printk 함수 포함
#include <linux/init.h>     // __init, __exit 매크로 정의

// 모듈이 커널에 삽입될 때 호출되는 함수
static int __init hello_init(void)
{
    printk(KERN_INFO "Hello, Kernel! - from Kim Hyo Jin's first module\n");
    return 0;
}

// 모듈이 커널에서 제거될 때 호출되는 함수
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye, Kernel! - from Kim Hyo Jin's module\n");
}

// 커널 모듈 진입/종료 함수 등록
module_init(hello_init);
module_exit(hello_exit);

// 모듈 메타 정보
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kim Hyo Jin");
MODULE_DESCRIPTION("First simple kernel module - hello_driver");

