#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../../kernel/blkcat_ioctl.h"
#include "mem.h"

static int dev = -1;
static uint32_t target_pid;

int mem_open(pid_t pid)
{
	dev = open("/dev/" BLKCAT_DEV_NAME, O_RDONLY);
	if (dev < 0)
		return -1;
	target_pid = (uint32_t)pid;
	return 0;
}

void mem_close(void)
{
	if (dev >= 0)
		close(dev);
	dev = -1;
}

bool mem_read_raw(uintptr_t addr, void *out, size_t n)
{
	struct blkcat_read_req req;

	memset(out, 0, n);
	if (dev < 0 || !addr || !n)
		return false;
	req.pid = target_pid;
	req._pad = 0;
	req.remote_addr = addr;
	req.user_buf = (uint64_t)(uintptr_t)out;
	req.size = n;
	req.result = 0;
	if (ioctl(dev, BLKCAT_IOCTL_READ_MEM, &req) < 0)
		return false;
	if (req.result != (int64_t)n) {
		memset(out, 0, n);
		return false;
	}
	return true;
}

uint8_t mem_u8(uintptr_t addr)
{
	uint8_t v;
	return mem_read_raw(addr, &v, sizeof v) ? v : 0;
}

uint32_t mem_u32(uintptr_t addr)
{
	uint32_t v;
	return mem_read_raw(addr, &v, sizeof v) ? v : 0;
}

int32_t mem_i32(uintptr_t addr)
{
	int32_t v;
	return mem_read_raw(addr, &v, sizeof v) ? v : 0;
}

uintptr_t mem_u64(uintptr_t addr)
{
	uint64_t v;
	return mem_read_raw(addr, &v, sizeof v) ? (uintptr_t)v : 0;
}

float mem_f32(uintptr_t addr)
{
	float v;
	return mem_read_raw(addr, &v, sizeof v) ? v : 0.0f;
}

int mem_mouse_button(int button, int down)
{
	struct blkcat_mouse_btn_req req = { (uint32_t)button, (uint32_t)down };

	if (dev < 0)
		return -1;
	return ioctl(dev, BLKCAT_IOCTL_MOUSE_BTN, &req);
}
