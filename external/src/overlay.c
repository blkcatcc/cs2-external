#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <X11/keysym.h>

#include "grab.h"
#include "overlay.h"

static Display *dpy;
static Window win;
static Pixmap back;
static GC gc;
static XFontStruct *font;
static int ow, oh;

static Visual *vis;
static Picture pic;
static Cursor arrow;
static Window saved_focus;
static bool have_saved_focus;
static char keys_now[32], keys_prev[32];
static bool mouse_grabbed;
static unsigned grabbed_btn;
static int own_x, own_y;
static bool own_valid;

static void set_cardinal(const char *name, long value)
{
	XChangeProperty(dpy, win, XInternAtom(dpy, name, False), XA_CARDINAL,
			32, PropModeReplace, (unsigned char *)&value, 1);
}

static bool is_gamescope(Display *d)
{
	static const char *const props[] = {
		"GAMESCOPE_DISPLAY_REFRESH_RATE_FEEDBACK",
		"GAMESCOPE_FOCUSED_WINDOW",
		"GAMESCOPE_INPUT_COUNTER",
	};
	size_t i;

	for (i = 0; i < sizeof props / sizeof props[0]; i++)
		if (XInternAtom(d, props[i], True) != None)
			return true;
	return false;
}

static long root_cardinal(Display *d, const char *name)
{
	Atom a = XInternAtom(d, name, True);
	Atom type;
	int fmt;
	unsigned long items, after;
	unsigned char *data = NULL;
	long v = -1;

	if (a == None)
		return -1;
	if (XGetWindowProperty(d, DefaultRootWindow(d), a, 0, 1, False,
			       XA_CARDINAL, &type, &fmt, &items, &after,
			       &data) != Success || !data)
		return -1;
	if (items >= 1 && fmt == 32)
		v = (long)*(unsigned long *)data;
	XFree(data);
	return v;
}

static int size_flag(const char *argv, const char *shortf, const char *longf)
{
	size_t sl = strlen(shortf), ll = strlen(longf);
	const char *a = argv;

	while (*a) {
		const char *v = NULL;
		size_t n = strlen(a);

		if (!strcmp(a, "--"))
			break;
		if (!strcmp(a, shortf) || !strcmp(a, longf))
			v = a + n + 1;
		else if (!strncmp(a, longf, ll) && a[ll] == '=')
			v = a + ll + 1;
		else if (!strncmp(a, shortf, sl) && n > sl)
			v = a + sl;
		a += n + 1;
		if (v && isdigit((unsigned char)*v))
			return atoi(v);
	}
	return 0;
}

static bool gamescope_output_size(Display *d, int *w, int *h)
{
	char path[64], buf[8192];
	long pid;
	FILE *f;
	size_t n;

	if (!is_gamescope(d))
		return false;
	pid = root_cardinal(d, "GAMESCOPE_PID");
	if (pid <= 0)
		return false;
	snprintf(path, sizeof path, "/proc/%ld/cmdline", pid);
	f = fopen(path, "rb");
	if (!f)
		return false;
	n = fread(buf, 1, sizeof buf - 1, f);
	fclose(f);
	buf[n] = '\0';

	*w = size_flag(buf, "-W", "--output-width");
	*h = size_flag(buf, "-H", "--output-height");
	return *w > 0 && *h > 0;
}

int overlay_init(int width, int height)
{
	const char *name = getenv("BLKCAT_DISPLAY");
	XVisualInfo tmpl, *vi;
	XSetWindowAttributes wa;
	Colormap cmap;
	int scr, n = 0;

	if (!name)
		name = OVERLAY_DEFAULT_DISPLAY;
	dpy = XOpenDisplay(name);
	if (!dpy) {
		fprintf(stderr, "blkcat: cannot open display %s\n", name);
		return -1;
	}

	ow = width;
	oh = height;
	scr = DefaultScreen(dpy);
	if (is_gamescope(dpy) &&
	    !gamescope_output_size(dpy, &ow, &oh)) {
		ow = DisplayWidth(dpy, scr);
		oh = DisplayHeight(dpy, scr);
	}

	memset(&tmpl, 0, sizeof tmpl);
	tmpl.screen = scr;
	tmpl.depth = 32;
	tmpl.class = TrueColor;
	vi = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask |
			    VisualClassMask, &tmpl, &n);
	if (!vi || n == 0) {
		if (vi)
			XFree(vi);
		XCloseDisplay(dpy);
		dpy = NULL;
		return -1;
	}
	cmap = XCreateColormap(dpy, RootWindow(dpy, scr), vi->visual, AllocNone);
	memset(&wa, 0, sizeof wa);
	wa.colormap = cmap;
	wa.background_pixel = 0;
	wa.border_pixel = 0;

	wa.override_redirect = True;
	win = XCreateWindow(dpy, RootWindow(dpy, scr), 0, 0, ow, oh, 0, 32,
			    InputOutput, vi->visual,
			    CWColormap | CWBackPixel | CWBorderPixel |
			    CWOverrideRedirect, &wa);
	vis = vi->visual;
	XFree(vi);
	back = XCreatePixmap(dpy, win, ow, oh, 32);
	{
		XRenderPictFormat *fmt = XRenderFindVisualFormat(dpy, vis);

		if (fmt)
			pic = XRenderCreatePicture(dpy, back, fmt, 0, NULL);
	}
	gc = XCreateGC(dpy, win, 0, NULL);
	font = XLoadQueryFont(dpy, "fixed");
	if (font)
		XSetFont(dpy, gc, font->fid);
	else
		fprintf(stderr, "blkcat: no 'fixed' font, ESP text disabled\n");

	set_cardinal("GAMESCOPE_EXTERNAL_OVERLAY", 1);
	set_cardinal("GAMESCOPE_NO_FOCUS", 1);
	XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, NULL, 0, ShapeSet,
				Unsorted);
	XMapWindow(dpy, win);
	XMoveResizeWindow(dpy, win, 0, 0, ow, oh);
	XRaiseWindow(dpy, win);
	XFlush(dpy);
	return 0;
}

int overlay_width(void)
{
	return ow;
}

int overlay_height(void)
{
	return oh;
}

void overlay_begin(void)
{
	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, back, gc, 0, 0, ow, oh);
}

void overlay_color(unsigned long rgb)
{
	XSetForeground(dpy, gc, 0xFF000000UL | (rgb & 0xFFFFFFUL));
}

void overlay_line_width(int px)
{
	if (px < 1)
		px = 1;
	XSetLineAttributes(dpy, gc, px, LineSolid, CapButt, JoinMiter);
}

void overlay_rect(int x, int y, int w, int h)
{
	XDrawRectangle(dpy, back, gc, x, y, (unsigned)w, (unsigned)h);
}

void overlay_fill_rect(int x, int y, int w, int h)
{
	XFillRectangle(dpy, back, gc, x, y, (unsigned)w, (unsigned)h);
}

void overlay_line(int x0, int y0, int x1, int y1)
{
	XDrawLine(dpy, back, gc, x0, y0, x1, y1);
}

void overlay_circle(int cx, int cy, int r)
{
	XDrawArc(dpy, back, gc, cx - r, cy - r, (unsigned)(2 * r), (unsigned)(2 * r),
		 0, 360 * 64);
}

void overlay_text(int x, int y, const char *s)
{
	if (font)
		XDrawString(dpy, back, gc, x, y, s, (int)strlen(s));
}

void overlay_text_size(const char *s, int *w, int *h)
{
	if (!font) {
		*w = 0;
		*h = 0;
		return;
	}
	*w = XTextWidth(font, s, (int)strlen(s));
	*h = font->ascent + font->descent;
}

void overlay_fill_rgba(int x, int y, int w, int h, unsigned long argb)
{
	unsigned a = (unsigned)((argb >> 24) & 0xFF);
	XRenderColor c;

	if (!pic || w <= 0 || h <= 0)
		return;

	c.alpha = (unsigned short)(a * 0x101);
	c.red = (unsigned short)((((argb >> 16) & 0xFF) * a / 0xFF) * 0x101);
	c.green = (unsigned short)((((argb >> 8) & 0xFF) * a / 0xFF) * 0x101);
	c.blue = (unsigned short)(((argb & 0xFF) * a / 0xFF) * 0x101);
	XRenderFillRectangle(dpy, PictOpOver, pic, &c, x, y, (unsigned)w,
			     (unsigned)h);
}

void overlay_fill_rects(const int *xywh, int n)
{
	static XRectangle r[2048];
	int i, m = n < 2048 ? n : 2048;

	for (i = 0; i < m; i++) {
		r[i].x = (short)xywh[4 * i + 0];
		r[i].y = (short)xywh[4 * i + 1];
		r[i].width = (unsigned short)xywh[4 * i + 2];
		r[i].height = (unsigned short)xywh[4 * i + 3];
	}
	XFillRectangles(dpy, back, gc, r, m);
}

void overlay_blit_rgba(int x, int y, int w, int h, const unsigned long *argb)
{
	static XImage *im;
	int i, j;

	if (!dpy || w <= 0 || h <= 0)
		return;
	if (im && (im->width != w || im->height != h)) {
		XDestroyImage(im);
		im = NULL;
	}
	if (!im) {
		size_t n = (size_t)w * 4u * (size_t)h;
		char *buf = calloc(1, n);

		if (!buf)
			return;
		im = XCreateImage(dpy, vis, 32, ZPixmap, 0, buf, (unsigned)w,
				  (unsigned)h, 32, 0);
		if (!im) {
			free(buf);
			return;
		}
	}
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			XPutPixel(im, i, j, argb[j * w + i]);
	XPutImage(dpy, back, gc, im, 0, 0, x, y, (unsigned)w, (unsigned)h);
}

static int key_bit(const char *k, int kc)
{
	return kc > 0 && kc < 256 && ((k[kc >> 3] >> (kc & 7)) & 1);
}

void overlay_keys_update(void)
{
	if (!dpy)
		return;
	memcpy(keys_prev, keys_now, sizeof keys_now);
	XQueryKeymap(dpy, keys_now);
}

int overlay_key_pressed(unsigned long keysym)
{
	int kc;

	if (!dpy || !keysym)
		return 0;
	kc = (int)XKeysymToKeycode(dpy, (KeySym)keysym);
	return key_bit(keys_now, kc) && !key_bit(keys_prev, kc);
}

int overlay_key_down(unsigned long keysym)
{
	int kc;

	if (!dpy || !keysym)
		return 0;
	kc = (int)XKeysymToKeycode(dpy, (KeySym)keysym);
	return key_bit(keys_now, kc);
}

const char *overlay_key_name(unsigned long keysym)
{
	return keysym ? XKeysymToString((KeySym)keysym) : NULL;
}

int overlay_keys_pressed(unsigned long *out, int max)
{
	int kc, n = 0;

	if (!dpy)
		return 0;
	for (kc = 8; kc < 256 && n < max; kc++)
		if (key_bit(keys_now, kc) && !key_bit(keys_prev, kc))
			out[n++] = (unsigned long)XkbKeycodeToKeysym(
				dpy, (KeyCode)kc, 0, 0);
	return n;
}

void overlay_input_grab(int on)
{
	XRectangle rect;

	if (!dpy || !win)
		return;
	if (on) {
		rect.x = 0;
		rect.y = 0;
		rect.width = (unsigned short)ow;
		rect.height = (unsigned short)oh;
		XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, &rect, 1,
					ShapeSet, Unsorted);
		XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask |
			     PointerMotionMask | EnterWindowMask | LeaveWindowMask);
		if (!arrow)
			arrow = XCreateFontCursor(dpy, XC_left_ptr);
		if (arrow)
			XDefineCursor(dpy, win, arrow);
		if (!have_saved_focus) {
			Window f;
			int rev;

			if (XGetInputFocus(dpy, &f, &rev) && f != None &&
			    f != PointerRoot && f != win) {
				saved_focus = f;
				have_saved_focus = true;
			}
		}

		XSetInputFocus(dpy, win, RevertToParent, CurrentTime);

		XGrabPointer(dpy, win, False,
			     ButtonPressMask | ButtonReleaseMask |
			     PointerMotionMask, GrabModeAsync, GrabModeAsync,
			     None, None, CurrentTime);
	} else {
		XUngrabPointer(dpy, CurrentTime);
		if (have_saved_focus) {
			XSetInputFocus(dpy, saved_focus, RevertToPointerRoot,
				       CurrentTime);
			have_saved_focus = false;
		} else {
			XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot,
				       CurrentTime);
		}
		XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, NULL, 0,
					ShapeSet, Unsorted);
		XSelectInput(dpy, win, 0);
		XUndefineCursor(dpy, win);
	}
	XFlush(dpy);
}

void overlay_input_refocus(void)
{
	static struct timespec last;
	struct timespec now;
	Window f;
	int rev;

	if (!dpy || !win)
		return;
	clock_gettime(CLOCK_MONOTONIC, &now);
	if (last.tv_sec && now.tv_sec - last.tv_sec < 1 &&
	    now.tv_nsec - last.tv_nsec < 250000000L)
		return;
	last = now;
	if (XGetInputFocus(dpy, &f, &rev) && f == win)
		return;
	XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
	XFlush(dpy);
}

void overlay_mouse_init(void)
{
	grab_open();
}

int overlay_mouse_grab(int on)
{
	if (on == mouse_grabbed)
		return mouse_grabbed;
	if (on) {
		if (grab_begin() > 0)
			mouse_grabbed = true;
		own_valid = false;
	} else {
		grab_end();
		mouse_grabbed = false;
		own_valid = false;
	}
	return mouse_grabbed;
}

void overlay_pointer(int *x, int *y, unsigned *buttons, int *wheel)
{
	Window rr, cr;
	int rx = 0, ry = 0, wx = 0, wy = 0, scr, sw, sh;
	unsigned mask = 0;

	*x = 0;
	*y = 0;
	if (buttons)
		*buttons = 0;
	if (wheel)
		*wheel = 0;
	if (!dpy || !win)
		return;
	scr = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, scr);
	sh = DisplayHeight(dpy, scr);
	XQueryPointer(dpy, win, &rr, &cr, &rx, &ry, &wx, &wy, &mask);

	if (sw > 0 && sh > 0) {
		*x = (int)((long)rx * ow / sw);
		*y = (int)((long)ry * oh / sh);
	}
	if (!mouse_grabbed) {
		if (buttons)
			*buttons = (mask & Button1Mask) ? 1u : 0u;
		return;
	}

	{
		int dx = 0, dy = 0, w = 0;

		grab_pump(&dx, &dy, &grabbed_btn, wheel ? wheel : &w);
		if (!own_valid) {
			own_x = *x;
			own_y = *y;
			own_valid = true;
		}
		if (sw > 0 && sh > 0) {
			own_x += (int)((long)dx * ow / sw);
			own_y += (int)((long)dy * oh / sh);
		}
		if (own_x < 0)
			own_x = 0;
		if (own_x >= ow)
			own_x = ow - 1;
		if (own_y < 0)
			own_y = 0;
		if (own_y >= oh)
			own_y = oh - 1;
		*x = own_x;
		*y = own_y;
		if (buttons)
			*buttons = grabbed_btn;

		if (dx || dy)
			XWarpPointer(dpy, None, RootWindow(dpy, scr), 0, 0, 0, 0,
				     dx, dy);
	}
}

void overlay_present(void)
{
	XCopyArea(dpy, back, win, gc, 0, 0, ow, oh, 0, 0);
	XFlush(dpy);
}

void overlay_exit(void)
{
	if (!dpy)
		return;
	overlay_mouse_grab(0);
	if (pic)
		XRenderFreePicture(dpy, pic);
	if (arrow)
		XFreeCursor(dpy, arrow);
	if (font)
		XFreeFont(dpy, font);
	XFreePixmap(dpy, back);
	XFreeGC(dpy, gc);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	dpy = NULL;
}
