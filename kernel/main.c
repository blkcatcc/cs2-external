// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "blkcat.h"

static long blkcat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case BLKCAT_IOCTL_READ_MEM: {
		struct blkcat_read_req req;

		if (copy_from_user(&req, uarg, sizeof(req)))
			return -EFAULT;
		req.result = blkcat_read_mem(&req);
		if (copy_to_user(uarg, &req, sizeof(req)))
			return -EFAULT;
		return 0;
	}
	case BLKCAT_IOCTL_MOUSE_BTN: {
		struct blkcat_mouse_btn_req req;

		if (copy_from_user(&req, uarg, sizeof(req)))
			return -EFAULT;
		return blkcat_mouse_button(&req);
	}
	case BLKCAT_IOCTL_MOUSE_CLICK: {
		struct blkcat_mouse_click_req req;

		if (copy_from_user(&req, uarg, sizeof(req)))
			return -EFAULT;
		return blkcat_mouse_click(&req);
	}
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
static long blkcat_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	return -ENOTTY;
}
#endif

static const struct file_operations blkcat_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= blkcat_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= blkcat_compat_ioctl,
#endif
};

static struct miscdevice blkcat_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= BLKCAT_DEV_NAME,
	.fops	= &blkcat_fops,
	.mode	= 0600,
};

static int __init blkcat_init(void)
{
	int err;

	err = blkcat_mouse_init();
	if (err)
		return err;

	err = misc_register(&blkcat_dev);
	if (err) {
		blkcat_mouse_exit();
		return err;
	}

	pr_info("blkcat: ready at /dev/%s\n", BLKCAT_DEV_NAME);
	return 0;
}

static void __exit blkcat_exit(void)
{
	misc_deregister(&blkcat_dev);
	blkcat_mouse_exit();
	pr_info("blkcat: unloaded\n");
}

module_init(blkcat_init);
module_exit(blkcat_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("blkcat");
MODULE_DESCRIPTION("CS2 multihack backend: process memory read + mouse click injection");
