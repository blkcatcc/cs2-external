/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLKCAT_H
#define BLKCAT_H

#include "blkcat_ioctl.h"

long blkcat_read_mem(struct blkcat_read_req *req);

int blkcat_mouse_button(struct blkcat_mouse_btn_req *req);
int blkcat_mouse_click(struct blkcat_mouse_click_req *req);
int blkcat_mouse_init(void);
void blkcat_mouse_exit(void);

#endif
