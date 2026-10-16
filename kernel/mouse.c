// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/module.h>

#include "blkcat.h"

static struct input_dev *blkcat_mouse;

static int blkcat_btn_code(__u32 button, unsigned int *code)
{
	switch (button) {
	case BLKCAT_BTN_LEFT:
		*code = BTN_LEFT;
		return 0;
	case BLKCAT_BTN_RIGHT:
		*code = BTN_RIGHT;
		return 0;
	case BLKCAT_BTN_MIDDLE:
		*code = BTN_MIDDLE;
		return 0;
	default:
		return -EINVAL;
	}
}

static int blkcat_emit(__u32 button, __u32 down)
{
	unsigned int code;
	int err = blkcat_btn_code(button, &code);

	if (err)
		return err;
	input_event(blkcat_mouse, EV_KEY, code, down ? 1 : 0);
	input_sync(blkcat_mouse);
	return 0;
}

int blkcat_mouse_button(struct blkcat_mouse_btn_req *req)
{
	return blkcat_emit(req->button, req->down);
}

int blkcat_mouse_click(struct blkcat_mouse_click_req *req)
{
	int err = blkcat_emit(req->button, 1);

	if (err)
		return err;
	if (req->hold_ms)
		msleep(req->hold_ms);
	return blkcat_emit(req->button, 0);
}

int blkcat_mouse_init(void)
{
	int err;

	blkcat_mouse = input_allocate_device();
	if (!blkcat_mouse)
		return -ENOMEM;

	blkcat_mouse->name = "blkcat virtual mouse";
	blkcat_mouse->id.bustype = BUS_USB;
	blkcat_mouse->id.vendor = 0x046d;
	blkcat_mouse->id.product = 0xc52b;

	__set_bit(EV_KEY, blkcat_mouse->evbit);
	__set_bit(EV_REL, blkcat_mouse->evbit);
	__set_bit(EV_SYN, blkcat_mouse->evbit);
	__set_bit(BTN_LEFT, blkcat_mouse->keybit);
	__set_bit(BTN_RIGHT, blkcat_mouse->keybit);
	__set_bit(BTN_MIDDLE, blkcat_mouse->keybit);

	__set_bit(REL_X, blkcat_mouse->relbit);
	__set_bit(REL_Y, blkcat_mouse->relbit);

	err = input_register_device(blkcat_mouse);
	if (err) {
		input_free_device(blkcat_mouse);
		blkcat_mouse = NULL;
		return err;
	}
	return 0;
}

void blkcat_mouse_exit(void)
{
	if (blkcat_mouse)
		input_unregister_device(blkcat_mouse);
	blkcat_mouse = NULL;
}
