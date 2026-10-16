/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLKCAT_IOCTL_H
#define BLKCAT_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define BLKCAT_IOC_MAGIC	0xB7

#define BLKCAT_DEV_NAME		"blkcat"

#define BLKCAT_BTN_LEFT		0
#define BLKCAT_BTN_RIGHT	1
#define BLKCAT_BTN_MIDDLE	2

struct blkcat_read_req {
	__u32 pid;
	__u32 _pad;
	__u64 remote_addr;
	__u64 user_buf;
	__u64 size;
	__s64 result;
};

struct blkcat_mouse_btn_req {
	__u32 button;
	__u32 down;
};

struct blkcat_mouse_click_req {
	__u32 button;
	__u32 hold_ms;
};

#define BLKCAT_IOCTL_READ_MEM \
	_IOWR(BLKCAT_IOC_MAGIC, 1, struct blkcat_read_req)
#define BLKCAT_IOCTL_MOUSE_BTN \
	_IOW(BLKCAT_IOC_MAGIC, 2, struct blkcat_mouse_btn_req)
#define BLKCAT_IOCTL_MOUSE_CLICK \
	_IOW(BLKCAT_IOC_MAGIC, 3, struct blkcat_mouse_click_req)

#endif
