#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>

#include <linux/platform_device.h>

#define DEV_NAME "Embedfire_LED"
#define DEV_COUNT 1

#define LED_ON 0
#define LED_OFF 1

struct LED_DEV {
    dev_t led_devid;
    struct cdev led_cdev;
    struct device *led_device;
    struct class *led_class;
    struct device_node *led_node;
};

struct LED_GPIO {
    unsigned int rled_gpio;
    unsigned int gled_gpio;
    unsigned int bled_gpio;
};

struct LED_GPIO *led_gpio;

/* file_operations 操作函数 */
static int led_open(struct inode *inode, struct file *file)
{
    /* 保存设备文件私有数据，防止同一主设备的子设备访问冲突 */
    file->private_data = led_gpio;

    printk("open complete!!\n");
    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{   
    file->private_data = NULL;

    printk("release complete!!\n");
	return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf,
		size_t cnt, loff_t *pos) 
{
    struct LED_GPIO *fileops_dev = (struct LED_GPIO *)file->private_data;

    int error; /* 用于保存接收到的数据 */
    unsigned char write_data[1];
     
	error = copy_from_user(write_data, buf, cnt);
	if(error < 0) {
		return -1;
	}

    /*设置 GPIO1_04 输出电平*/
	if(write_data[0] == 1) {
		gpio_set_value(fileops_dev->rled_gpio, LED_ON);  // GPIO1_04引脚输出低电平，红灯亮
	}
	else if (write_data[0] == 2) {
		gpio_set_value(fileops_dev->rled_gpio, LED_OFF);    // GPIO1_04引脚输出高电平，红灯灭
	}

    /*设置 GPIO4_20 输出电平*/
	if(write_data[0] == 3) {
		gpio_set_value(fileops_dev->gled_gpio, LED_ON);  // GPIO4_20引脚输出低电平，绿灯亮
	}
	else if (write_data[0] == 4){
		gpio_set_value(fileops_dev->gled_gpio, LED_OFF);    // GPIO4_20引脚输出高电平，绿灯灭
	}

    /*设置 GPIO4_19 输出电平*/
	if(write_data[0] == 5) {
		gpio_set_value(fileops_dev->bled_gpio, LED_ON);  // GPIO4_19引脚输出低电平，蓝灯亮
	}
	else if (write_data[0] == 6){
		gpio_set_value(fileops_dev->bled_gpio, LED_OFF);    // GPIO4_19引脚输出高电平，蓝灯灭
	}

	return 0;
}

static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};


/* .probe .remove 操作函数*/
static int LED_probe(struct platform_device *pdev) 
{   
    int ret = 0;

    /* 为LED设备分配内存  */
    struct LED_DEV *led_dev;
    led_dev = devm_kzalloc(&pdev->dev,sizeof(struct LED_DEV), GFP_KERNEL);
	if (!led_dev)
		return -1;

    /* 为GPIO分配内存 */
    led_gpio = kzalloc(sizeof(struct LED_GPIO), GFP_KERNEL);
	if (!led_gpio)
		return -1;

    /* 获取设备树节点 */
    led_dev->led_node = of_find_node_by_path("/Embedfire_LED");
    if(led_dev->led_node == NULL) {
        printk(KERN_EMERG "\t  get rgb_led failed!  \n");
    }

    /* 从设备树节点中获取设备GPIO编号 */
    led_gpio->rled_gpio = of_get_named_gpio(led_dev->led_node, "rgb_led_red", 0);
    if(led_gpio->rled_gpio < 0) return led_gpio->rled_gpio;
    led_gpio->gled_gpio = of_get_named_gpio(led_dev->led_node, "rgb_led_green", 0);
    if(led_gpio->gled_gpio < 0) return led_gpio->gled_gpio;
    led_gpio->bled_gpio = of_get_named_gpio(led_dev->led_node, "rgb_led_blue", 0);
    if(led_gpio->bled_gpio < 0) return led_gpio->bled_gpio;

    printk("rgb_led_red = %d \n rgb_led_green = %d \n rgb_led_blue = %d \n",\
            led_gpio->rled_gpio,led_gpio->gled_gpio,led_gpio->bled_gpio);

    /* 对应GPIO编号引脚设置为输出模式,灯默认为熄灭状态 */
    ret = gpio_direction_output(led_gpio->rled_gpio , LED_ON);
    if(ret < 0) return ret;
    ret = gpio_direction_output(led_gpio->gled_gpio , LED_OFF);
    if(ret < 0) return ret;
    ret = gpio_direction_output(led_gpio->bled_gpio , LED_ON);
    if(ret < 0) return ret;

    /* 注册字符设备、类、设备 */
	ret = alloc_chrdev_region(&led_dev->led_devid, 0, DEV_COUNT,DEV_NAME);
	if (ret < 0)    goto alloc_failed;

    led_dev->led_cdev.owner = THIS_MODULE;
    cdev_init(&led_dev->led_cdev, &led_fops);

	ret = cdev_add(&led_dev->led_cdev, led_dev->led_devid, DEV_COUNT);
    if (ret < 0)    goto cdev_add_failed;

    led_dev->led_class = class_create(THIS_MODULE, "LED_DEV");
	if (IS_ERR(led_dev->led_class))     goto class_create_failed;

    led_dev->led_device = device_create(led_dev->led_class, NULL, led_dev->led_devid, NULL, DEV_NAME);
	if (IS_ERR(led_dev->led_device))    goto device_create_failed;

    /* 保存LED设备 */
    platform_set_drvdata(pdev, led_dev);

    printk("LED_probe complete!!\n");
	return 0;

device_create_failed:
    device_destroy(led_dev->led_class, led_dev->led_devid);
class_create_failed:
    class_destroy(led_dev->led_class);
cdev_add_failed:
    cdev_del(&led_dev->led_cdev);
alloc_failed:
    unregister_chrdev_region(led_dev->led_devid, DEV_COUNT);

    printk("LED_probe failed!!\n");
    return -1;
}

static int LED_remove(struct platform_device *pdev)
{
    /* 提取LED设备指针 */
    struct LED_DEV *led_dev = platform_get_drvdata(pdev);

    /* ##### 新增 ##### 退出模块时关灯 */
    gpio_set_value(led_gpio->rled_gpio, LED_OFF);
    gpio_set_value(led_gpio->gled_gpio, LED_OFF);
    gpio_set_value(led_gpio->bled_gpio, LED_OFF);

    /* 注销字符设备、类、设备 */
    device_destroy(led_dev->led_class, led_dev->led_devid);
    class_destroy(led_dev->led_class);
    cdev_del(&led_dev->led_cdev);
    unregister_chrdev_region(led_dev->led_devid, DEV_COUNT);

    /* 释放内存 */
    kfree(led_gpio);
    platform_set_drvdata(pdev, NULL);

    printk("LED_remove complete\n");
    return 0;
}


/* 设备树匹配表 */
static const struct of_device_id LED_match_table[] = {
	{ .compatible = "fire,Embedfire_LED" },
	{}
};


/* 平台驱动结构体 */
struct platform_driver LED_platform_driver = {
    .probe = LED_probe,
    .remove = LED_remove,
    .driver = {
        .name = "LED_driver",
        .owner = THIS_MODULE,
        .of_match_table = LED_match_table,
    }
};


/* 加载、卸载模块 */
static int __init Embedfire_LED_init(void) 
{
    /* 注册平台驱动 */
    platform_driver_register(&LED_platform_driver);
    
    return 0;
}

static void __exit Embedfire_LED_exit(void) 
{   
    /* 卸载平台驱动 */
    platform_driver_unregister(&LED_platform_driver);

}

module_init(Embedfire_LED_init);
module_exit(Embedfire_LED_exit);

MODULE_LICENSE("GPL");
