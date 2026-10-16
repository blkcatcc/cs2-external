#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "grab.h"

#define MAXDEV 8
#define NBITS (8 * (int)sizeof(unsigned long))

static int fds[MAXDEV];
static int nfds;
static unsigned buttons;
static bool opened;

static bool has_bit(const unsigned long *b, int bit)
{
	return (b[bit / NBITS] >> (bit % NBITS)) & 1UL;
}

static unsigned long rel_bits[REL_MAX / NBITS + 1];

static bool is_pointer(int fd)
{
	memset(rel_bits, 0, sizeof rel_bits);
	if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof rel_bits), rel_bits) < 0)
		return false;
	return has_bit(rel_bits, REL_X) && has_bit(rel_bits, REL_Y);
}

int grab_open(void)
{
	char path[64 + sizeof(((struct dirent *)0)->d_name)], name[128];
	struct dirent *e;
	int denied = 0;
	DIR *d;

	if (nfds || opened)
		return nfds;
	opened = true;
	d = opendir("/dev/input");
	if (!d) {
		fprintf(stderr, "blkcat: mouse: cannot open /dev/input (%s)\n",
			strerror(errno));
		return 0;
	}
	while ((e = readdir(d)) && nfds < MAXDEV) {
		int fd;

		if (strncmp(e->d_name, "event", 5))
			continue;
		snprintf(path, sizeof path, "/dev/input/%s", e->d_name);
		fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
		if (fd < 0) {
			if (errno == EACCES)
				denied = errno;
			continue;
		}

		if (ioctl(fd, EVIOCGNAME(sizeof name), name) < 0 ||
		    strstr(name, "blkcat") || !is_pointer(fd)) {
			close(fd);
			continue;
		}
		fds[nfds++] = fd;
	}
	closedir(d);
	if (!nfds)
		fprintf(stderr, denied
					? "blkcat: mouse: /dev/input is not readable"
					  " (%s) - the game keeps the mouse\n"
					: "blkcat: mouse: no relative pointer in"
					  " /dev/input\n",
			denied ? strerror(denied) : "");
	return nfds;
}

static void drain(void)
{
	struct input_event ev;
	int i;

	for (i = 0; i < nfds; i++)
		while (read(fds[i], &ev, sizeof ev) == (ssize_t)sizeof ev)
			;
}

int grab_begin(void)
{
	int i, n = 0, err = 0;

	grab_open();
	for (i = 0; i < nfds; i++) {
		if (ioctl(fds[i], EVIOCGRAB, 1) == 0)
			n++;
		else
			err = errno;
	}
	buttons = 0;
	drain();
	if (nfds && !n)
		fprintf(stderr, "blkcat: mouse: EVIOCGRAB failed (%s)\n",
			strerror(err));
	return n;
}

void grab_end(void)
{
	int i;

	for (i = 0; i < nfds; i++)
		ioctl(fds[i], EVIOCGRAB, 0);
	buttons = 0;
}

static void fold_event(const struct input_event *ev, int *dx, int *dy,
		       int *wheel)
{
	if (ev->type == EV_REL && ev->code == REL_X)
		*dx += ev->value;
	else if (ev->type == EV_REL && ev->code == REL_Y)
		*dy += ev->value;
	else if (ev->type == EV_REL && ev->code == REL_WHEEL)
		*wheel += ev->value;
	else if (ev->type == EV_KEY && ev->code == BTN_LEFT)
		buttons = ev->value ? buttons | 1u : buttons & ~1u;
	else if (ev->type == EV_KEY && ev->code == BTN_RIGHT)
		buttons = ev->value ? buttons | 2u : buttons & ~2u;
	else if (ev->type == EV_KEY && ev->code == BTN_MIDDLE)
		buttons = ev->value ? buttons | 4u : buttons & ~4u;
}

void grab_pump(int *dx, int *dy, unsigned *btn, int *wheel)
{
	struct input_event ev;
	int i;

	*dx = 0;
	*dy = 0;
	*wheel = 0;
	for (i = 0; i < nfds; i++)
		while (read(fds[i], &ev, sizeof ev) == (ssize_t)sizeof ev)
			fold_event(&ev, dx, dy, wheel);
	*btn = buttons;
}
