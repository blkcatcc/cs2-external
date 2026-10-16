// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include "blkcat.h"

#define BLKCAT_CHUNK		PAGE_SIZE

#define BLKCAT_MAX_READ		(64UL * 1024 * 1024)

long blkcat_read_mem(struct blkcat_read_req *req)
{
	struct pid *pid;
	struct task_struct *task;
	void *kbuf;
	size_t done = 0;
	long ret = 0;

	if (!req->size || req->size > BLKCAT_MAX_READ)
		return -EINVAL;
	if (!req->remote_addr || !req->user_buf)
		return -EINVAL;
	if (!access_ok((void __user *)(unsigned long)req->user_buf, req->size))
		return -EFAULT;

	pid = find_get_pid((int)req->pid);
	if (!pid)
		return -ESRCH;
	task = get_pid_task(pid, PIDTYPE_PID);
	put_pid(pid);
	if (!task)
		return -ESRCH;

	kbuf = kmalloc(BLKCAT_CHUNK, GFP_KERNEL);
	if (!kbuf) {
		put_task_struct(task);
		return -ENOMEM;
	}

	while (done < req->size) {
		size_t want = min_t(size_t, req->size - done, BLKCAT_CHUNK);
		int n = access_process_vm(task, req->remote_addr + done, kbuf,
					  (int)want, FOLL_FORCE);

		if (n <= 0)
			break;
		if (copy_to_user((void __user *)(unsigned long)
				 (req->user_buf + done), kbuf, n)) {
			ret = -EFAULT;
			break;
		}
		done += n;
		if ((size_t)n < want)
			break;
	}

	kfree(kbuf);
	put_task_struct(task);
	return ret ? ret : (long)done;
}
