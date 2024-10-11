#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0xdcb764ad, "memset" },
	{ 0x69907a56, "gpio_to_desc" },
	{ 0x695ce26c, "gpiod_set_raw_value" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x7dea1285, "try_module_get" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xe78e8907, "cdev_init" },
	{ 0x39f012c6, "cdev_add" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xed199bb0, "gpiod_direction_output_raw" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xa4afd25e, "cdev_del" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6abf2d87, "module_put" },
	{ 0x67a35d9, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "EAD8F90188724EF8C9D473D");
