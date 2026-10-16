#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "menu.h"
#include "overlay.h"
#include "pixfont.h"

#define KEY_F11		0xFFC8UL
#define KEY_ESC		0xFF1BUL
#define KEY_ENTER	0xFF0DUL
#define KEY_KP_ENTER	0xFF8DUL
#define KEY_BACKSPACE	0xFF08UL

#define C_GREEN		0x44DD44UL
#define C_GREEN_D	0x1C7A1CUL
#define C_PINK		0xFF9EB5UL
#define C_GREY		0xB4B4B4UL
#define C_GREY_D	0x5A5A5AUL
#define C_TEXT		0xF2F2F2UL
#define C_HAIR		0x333333UL
#define C_TRACK		0x161616UL
#define C_CAT_OUT	0x000000UL
#define C_CAT_BODY	0x1C1C1CUL
#define C_CAT_GREY	0x6E6E6EUL
#define C_CAT_NOSE	0xD98A9EUL
#define C_CAT_WHISK	0xC8C8C8UL

#define A_SCRIM		0x66B4B4B4UL
#define A_PANEL		0xD80A0A0AUL
#define A_HOVER		0x24FFFFFFUL

#define CAT_W 26
#define CAT_H 26
#define ICON_W 7
#define ICON_H 7

#define N(a) (sizeof (a) / sizeof (a)[0])

struct item {
	const char *label;
	int kind;
	bool *b;
	int *i;
	unsigned long *c;
	int lo, hi;
};

struct page {
	const char *name;
	const char *const *icon;
	const struct item *items;
	int nitems;
	bool picker;
	const char *unit;
};

enum { I_BOOL, I_INT, I_COLOR, I_KEY };

static const char *const k_cat[CAT_H] = {
	"......oo..........oo......",
	".....okko........okko.....",
	"....okkkkooooooookkkko....",
	"....okkkkkkkkkkkkkkkko....",
	"....okkkkkkkkkkkkkkkko....",
	"....oggggggggggggggggo....",
	"....oggggggggggggggggo....",
	"....okkkkkkkkkkkkkkkko....",
	"....okkkkekkkkkkekkkko....",
	"....okkkeeekkkkeeekkko....",
	"....okkkkekkkkkkekkkko....",
	".ww.okkkkkkkkkkkkkkkko.ww.",
	"....okkkkkkknnkkkkkkko....",
	".ww.okkkkkkkkkkkkkkkko.ww.",
	".....okkkkkkkkkkkkkko.....",
	".....oooooooooooooooo.....",
	"......okkkkkkkkkkkko......",
	".....okkkkkkkkkkkkkko.....",
	"....okkkkkkkkkkkkkkkko....",
	"...okkkkkkkkkkkkkkkkkko...",
	"...okkkkkkkkggkkkkkkkko...",
	"...okkkkkkkkggkkkkkkkko...",
	"...okkkkkkkkggkkkkkkkko...",
	"...okkkkkkkkggkkkkkkkko...",
	"...okkkkkkkkggkkkkkkkko...",
	"...oooooooooooooooooooo...",
};

static const char *const k_icon_play[ICON_H] = {
	"x......",
	"xx.....",
	"xxx....",
	"xxxx...",
	"xxx....",
	"xx.....",
	"x......",
};

static const char *const k_icon_eye[ICON_H] = {
	".......",
	"..xxx..",
	".x...x.",
	"x..x..x",
	".x...x.",
	"..xxx..",
	".......",
};

static const char *const k_icon_x[ICON_H] = {
	"x.....x",
	".x...x.",
	"..x.x..",
	"...x...",
	"..x.x..",
	".x...x.",
	"x.....x",
};

static const char *const k_icon_cross[ICON_H] = {
	".......",
	"...x...",
	"...x...",
	"xxx.xxx",
	"...x...",
	"...x...",
	".......",
};

static const char *const k_cursor[11] = {
	"o......",
	"oo.....",
	"oxo....",
	"oxxo...",
	"oxxxo..",
	"oxxxxo.",
	"oxxxxxo",
	"oxxxxoo",
	"oxo.oxo",
	"oo..oxo",
	".....oo",
};

static const struct item k_esp[] = {
	{ "Box", I_BOOL, &g_cfg.box, NULL, NULL, 0, 0 },
	{ "Full Box", I_BOOL, &g_cfg.full_box, NULL, NULL, 0, 0 },
	{ "Box Width", I_INT, NULL, &g_cfg.esp_thickness, NULL, 1, 8 },
	{ "Outline", I_BOOL, &g_cfg.esp_outline, NULL, NULL, 0, 0 },
	{ "Outline Width", I_INT, NULL, &g_cfg.esp_outline_thickness, NULL, 1, 8 },
	{ "Skeleton", I_BOOL, &g_cfg.esp_skeleton, NULL, NULL, 0, 0 },
};

static const struct item k_visuals[] = {
	{ "Name", I_BOOL, &g_cfg.name, NULL, NULL, 0, 0 },
	{ "Health Bar", I_BOOL, &g_cfg.health_bar, NULL, NULL, 0, 0 },
	{ "Health Text", I_BOOL, &g_cfg.health_text, NULL, NULL, 0, 0 },
	{ "Weapon", I_BOOL, &g_cfg.weapon, NULL, NULL, 0, 0 },
	{ "Distance", I_BOOL, &g_cfg.distance, NULL, NULL, 0, 0 },
	{ "Head Circle", I_BOOL, &g_cfg.head_circle, NULL, NULL, 0, 0 },
};

static const struct item k_colours[] = {
	{ "Box", I_COLOR, NULL, NULL, &g_cfg.box_color, 0, 0 },
	{ "Outline", I_COLOR, NULL, NULL, &g_cfg.outline_color, 0, 0 },
	{ "Skeleton", I_COLOR, NULL, NULL, &g_cfg.skeleton_color, 0, 0 },
	{ "Head Circle", I_COLOR, NULL, NULL, &g_cfg.head_circle_color, 0, 0 },
	{ "Name", I_COLOR, NULL, NULL, &g_cfg.name_color, 0, 0 },
	{ "Health", I_COLOR, NULL, NULL, &g_cfg.health_color, 0, 0 },
	{ "Health Text", I_COLOR, NULL, NULL, &g_cfg.health_text_color, 0, 0 },
	{ "Weapon", I_COLOR, NULL, NULL, &g_cfg.weapon_color, 0, 0 },
	{ "Distance", I_COLOR, NULL, NULL, &g_cfg.distance_color, 0, 0 },
};

static const struct item k_trigger[] = {
	{ "Trigger", I_BOOL, &g_cfg.triggerbot, NULL, NULL, 0, 0 },
	{ "Hotkey", I_KEY, NULL, &g_cfg.trigger_hotkey, NULL, 0, 0 },
	{ "Delay", I_INT, NULL, &g_cfg.trigger_delay, NULL, 0, 1000 },
	{ "Delay Jitter", I_INT, NULL, &g_cfg.trigger_jitter, NULL, 0, 100 },
	{ "Jitter %", I_BOOL, &g_cfg.trigger_jitter_pct, NULL, NULL, 0, 0 },
	{ "Hold", I_INT, NULL, &g_cfg.trigger_hold, NULL, 1, 500 },
	{ "Hold Jitter", I_INT, NULL, &g_cfg.trigger_hold_jitter, NULL, 0, 100 },
	{ "Next Shot", I_INT, NULL, &g_cfg.trigger_next, NULL, 0, 1000 },
};

static const struct page k_pages[] = {
	{ "ESP", k_icon_play, k_esp, (int)N(k_esp), false, "px" },
	{ "Visuals", k_icon_eye, k_visuals, (int)N(k_visuals), false, "px" },
	{ "Colours", k_icon_x, k_colours, (int)N(k_colours), true, "" },
	{ "Trigger", k_icon_cross, k_trigger, (int)N(k_trigger), false, "ms" },
};

#define SV_W 180
#define SV_H 150
#define HUE_W 22

struct layout {
	int k, border, pad, mx, my, mw, mh, hh, gap, nw;
	int cx, cy, cw, ch;
	int scale, title, cat, nav_h, sw_w, sw_h, sl_w, sl_h, val_w;
	int pk_x, pk_w, pk_y;
};

static bool g_open;
static int g_page;
static int g_scroll;
static int g_drag = -1;
static bool g_dirty;
static unsigned g_prev_btn;
static int g_border = 3;

static int g_sel;
static int g_pick;
static float g_h, g_s, g_v;
static unsigned long g_sv[SV_W * SV_H];
static unsigned long g_bar[HUE_W * SV_H];
static float g_sv_hue = -1.f;
static bool g_bar_built;
static bool g_pick_synced;

static bool g_edit;
static int g_edit_sel;
static char g_buf[8];
static int g_buf_n;
static int *g_capture;

static int g_mx, g_my;
static bool g_mouse_ours;

static void sync_picker(void);
static void start_edit(int row);

static unsigned long rgb(unsigned r, unsigned g, unsigned b)
{
	return 0xFF000000UL | (r << 16) | (g << 8) | b;
}

static void rgb_to_hsv(unsigned long c, float *h, float *s, float *v)
{
	float r = (float)((c >> 16) & 0xFF) / 255.f;
	float g = (float)((c >> 8) & 0xFF) / 255.f;
	float b = (float)(c & 0xFF) / 255.f;
	float mx = r > g ? r : g, mn = r < g ? r : g, d;

	if (mx < b)
		mx = b;
	if (mn > b)
		mn = b;
	d = mx - mn;
	*v = mx;
	*s = mx > 0.f ? d / mx : 0.f;
	if (d == 0.f) {
		*h = 0.f;
		return;
	}
	if (mx == r)
		*h = (g - b) / d;
	else if (mx == g)
		*h = (b - r) / d + 2.f;
	else
		*h = (r - g) / d + 4.f;
	*h /= 6.f;
	if (*h < 0.f)
		*h += 1.f;
}

static unsigned long hsv_to_rgb(float h, float s, float v)
{
	float f, p, q, t, r, g, b;
	int i;

	if (h < 0.f)
		h = 0.f;
	if (h >= 1.f)
		h = 0.9999f;
	f = h * 6.f;
	i = (int)f;
	f -= (float)i;
	p = v * (1.f - s);
	q = v * (1.f - s * f);
	t = v * (1.f - s * (1.f - f));
	switch (i) {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	default: r = v; g = p; b = q; break;
	}
	return rgb((unsigned)(r * 255.f + 0.5f), (unsigned)(g * 255.f + 0.5f),
		   (unsigned)(b * 255.f + 0.5f));
}

static void fill(int x, int y, int w, int h, unsigned long col)
{
	if (w <= 0 || h <= 0)
		return;
	overlay_color(col);
	overlay_fill_rect(x, y, w, h);
}

static void frame(int x, int y, int w, int h, unsigned long col, int th)
{
	fill(x + th, y, w - 2 * th, th, col);
	fill(x + th, y + h - th, w - 2 * th, th, col);
	fill(x, y + th, th, h - 2 * th, col);
	fill(x + w - th, y + th, th, h - 2 * th, col);
}

static void sprite_draw(int ox, int oy, int s, const char *const *grid, int rows,
			int cols, char glyph, unsigned long col)
{
	static int r[1024 * 4];
	int n = 0, y, x;

	for (y = 0; y < rows; y++) {
		for (x = 0; x < cols && n < 1024; ) {
			int run;

			if (grid[y][x] != glyph) {
				x++;
				continue;
			}
			for (run = 0; x + run < cols && grid[y][x + run] == glyph;
			     run++)
				;
			r[4 * n + 0] = ox + x * s;
			r[4 * n + 1] = oy + y * s;
			r[4 * n + 2] = run * s;
			r[4 * n + 3] = s;
			n++;
			x += run;
		}
	}
	if (n) {
		overlay_color(col);
		overlay_fill_rects(r, n);
	}
}

static void layout(struct layout *L)
{
	int W = overlay_width(), H = overlay_height(), k = H / 1080;

	if (k < 1)
		k = 1;
	L->k = k;
	L->border = 3 * k;
	g_border = L->border;
	L->pad = 26 * k;
	L->mx = 70 * k;
	L->my = 46 * k;
	L->mw = W - 2 * L->mx;
	L->mh = H - 2 * L->my;
	L->hh = 186 * k;
	L->gap = 22 * k;
	L->nw = 320 * k;
	L->cx = L->mx + L->nw + L->gap;
	L->cy = L->my + L->hh + L->gap;
	L->cw = L->mw - L->nw - L->gap;
	L->ch = L->mh - L->hh - L->gap;
	L->scale = 4 * k;
	L->title = 7 * k;
	L->cat = 5 * k;
	L->nav_h = 84 * k;
	L->sw_w = 150 * k;
	L->sw_h = 52 * k;
	L->sl_w = 400 * k;
	L->sl_h = 30 * k;
	L->val_w = 170 * k;

	L->pk_w = SV_W + 12 + HUE_W;
	L->pk_x = L->cx + L->cw - L->pad - L->pk_w;
	L->pk_y = L->cy + L->border + L->pad;
}

static int row_h_of(const struct layout *L, int nitems)
{
	int avail = L->ch - 2 * L->border - L->pad / 2;
	int h = avail / (nitems > 0 ? nitems : 1);

	if (h > 120 * L->k)
		h = 120 * L->k;
	if (h < 56 * L->k)
		h = 56 * L->k;
	return h;
}

static int visible_rows(const struct layout *L, int nitems)
{
	int avail = L->ch - 2 * L->border - L->pad / 2;
	int n = avail / row_h_of(L, nitems);

	if (n > nitems)
		n = nitems;
	return n < 1 ? 1 : n;
}

static int nav_hit(const struct layout *L, int x, int y)
{
	int y0 = L->cy + L->border + L->pad;
	size_t i;

	if (x < L->mx || x >= L->mx + L->nw)
		return -1;
	for (i = 0; i < N(k_pages); i++, y0 += L->nav_h)
		if (y >= y0 && y < y0 + L->nav_h)
			return (int)i;
	return -1;
}

static int row_hit(const struct layout *L, int x, int y)
{
	const struct page *p = &k_pages[g_page];
	int rh = row_h_of(L, p->nitems);
	int y0 = L->cy + L->border + L->pad / 2;
	int i;

	if (x < L->cx || x >= L->cx + L->cw || y < y0)
		return -1;
	i = (y - y0) / rh + g_scroll;
	if (i >= p->nitems || i >= g_scroll + visible_rows(L, p->nitems))
		return -1;
	return i;
}

static int picker_hit(const struct layout *L, int x, int y)
{
	if (y < L->pk_y || y >= L->pk_y + SV_H)
		return 0;
	if (x >= L->pk_x && x < L->pk_x + SV_W)
		return 1;
	if (x >= L->pk_x + SV_W + 12 && x < L->pk_x + SV_W + 12 + HUE_W)
		return 2;
	return 0;
}

static bool hit(int x, int y, int rx, int ry, int rw, int rh)
{
	return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static void row_hex_rect(const struct layout *L, int i, int *x, int *y, int *w,
			 int *h)
{
	const struct page *p = &k_pages[g_page];
	int rh = row_h_of(L, p->nitems);
	int rx1 = p->picker ? L->pk_x - L->pad : L->cx + L->cw - L->pad;
	int ry = L->cy + L->border + L->pad / 2 + (i - g_scroll) * rh;
	int tw = pixfont_width("000000", L->scale);

	*w = tw + L->pad;
	*h = PIXFONT_H * L->scale + L->pad;
	*x = rx1 - 34 * L->k - L->pad - tw - L->pad / 2;
	*y = ry + (rh - *h) / 2;
}

static void picker_hex_rect(const struct layout *L, int *x, int *y, int *w,
			    int *h)
{
	int sw = 34 * L->k;

	*w = pixfont_width("000000", L->scale) + L->pad;
	*h = PIXFONT_H * L->scale + L->pad;
	*x = L->pk_x;
	*y = L->pk_y + SV_H + L->pad + sw + L->pad / 2;
}

static void build_sv(void)
{
	unsigned long c = hsv_to_rgb(g_h, 1.f, 1.f);
	float hr = (float)((c >> 16) & 0xFF);
	float hg = (float)((c >> 8) & 0xFF);
	float hb = (float)(c & 0xFF);
	float v, t;
	int i, j;

	if (g_sv_hue == g_h)
		return;
	g_sv_hue = g_h;
	for (j = 0; j < SV_H; j++) {
		v = 255.f * (1.f - (float)j / (float)(SV_H - 1));
		for (i = 0; i < SV_W; i++) {
			t = v * (float)i / (float)(SV_W - 1);
			g_sv[j * SV_W + i] = rgb(
				(unsigned)(v - t + t * hr / 255.f + 0.5f),
				(unsigned)(v - t + t * hg / 255.f + 0.5f),
				(unsigned)(v - t + t * hb / 255.f + 0.5f));
		}
	}
}

static void build_bar(void)
{
	int i, j;

	if (g_bar_built)
		return;
	g_bar_built = true;
	for (j = 0; j < SV_H; j++) {
		unsigned long c = hsv_to_rgb((float)j / (float)(SV_H - 1), 1.f, 1.f);

		for (i = 0; i < HUE_W; i++)
			g_bar[j * HUE_W + i] = c;
	}
}

static void draw_switch(int x, int y, int w, int h, int on)
{
	int b = g_border, k = (w - 2 * b) * 3 / 5;
	int kx = on ? x + w - b - k : x + b;

	fill(x, y, w, h, C_TRACK);
	fill(kx, y + b, k, h - 2 * b, on ? C_PINK : C_GREY_D);
	frame(x, y, w, h, C_GREEN, b);
}

static void draw_slider(int x, int y, int w, int h, int pos, int n)
{
	int b = g_border, track = w - 2 * b;
	int fx = n > 1 ? (int)((long)track * pos / (n - 1)) : 0;
	int hx = x + b + fx - h / 2;

	fill(x, y, w, h, C_TRACK);
	fill(x + b, y + b, fx, h - 2 * b, C_GREEN_D);
	if (hx < x)
		hx = x;
	if (hx > x + w - h)
		hx = x + w - h;
	fill(hx, y, h, h, C_GREEN);
	frame(x, y, w, h, C_GREEN, b);
}

static void draw_scrim(void)
{
	overlay_fill_rgba(0, 0, overlay_width(), overlay_height(), A_SCRIM);
}

static void draw_hex(const struct layout *L, int x, int y, int w, int h,
		     unsigned long col, int row, unsigned long normal)
{
	char buf[16];
	int editing = g_edit && g_edit_sel == row;

	if (editing)
		snprintf(buf, sizeof buf, "%s_", g_buf);
	else
		snprintf(buf, sizeof buf, "%06lX", col);
	if (editing || hit(g_mx, g_my, x, y, w, h))
		frame(x, y, w, h, C_GREEN, 1);
	pixfont_draw(x + L->pad / 2, y + L->pad / 2, L->scale,
		     editing ? C_GREEN : normal, buf);
}

static void draw_header(const struct layout *L)
{
	int x = L->mx + L->border + L->pad;
	int y = L->my + (L->hh - CAT_H * L->cat) / 2;

	overlay_fill_rgba(L->mx, L->my, L->mw, L->hh, A_PANEL);
	frame(L->mx, L->my, L->mw, L->hh, C_GREEN, L->border);

	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'o', C_CAT_OUT);
	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'k', C_CAT_BODY);
	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'g', C_CAT_GREY);
	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'e', C_GREEN);
	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'n', C_CAT_NOSE);
	sprite_draw(x, y, L->cat, k_cat, CAT_H, CAT_W, 'w', C_CAT_WHISK);

	x += CAT_W * L->cat + L->pad;
	y = L->my + (L->hh - PIXFONT_H * L->title) / 2;
	pixfont_draw(x, y, L->title, C_GREEN, "BLKCAT.CC");

	x += pixfont_width("BLKCAT.CC", L->title) + L->pad;
	y += PIXFONT_H * (L->title - L->scale);
	pixfont_draw(x, y, L->scale, C_GREY, "- A DRIVER BASED EXTERNAL");
}

static void draw_nav(const struct layout *L)
{
	int y = L->cy + L->border + L->pad;
	int hov = nav_hit(L, g_mx, g_my);
	size_t i;

	overlay_fill_rgba(L->mx, L->cy, L->nw, L->ch, A_PANEL);
	frame(L->mx, L->cy, L->nw, L->ch, C_GREEN, L->border);

	for (i = 0; i < N(k_pages); i++) {
		unsigned long col = (int)i == g_page ? C_GREEN : C_GREY;

		if ((int)i == hov)
			overlay_fill_rgba(L->mx + L->border, y,
					  L->nw - 2 * L->border, L->nav_h,
					  A_HOVER);
		sprite_draw(L->mx + L->pad,
			    y + (L->nav_h - ICON_H * L->scale) / 2, L->scale,
			    k_pages[i].icon, ICON_H, ICON_W, 'x', col);
		pixfont_draw(L->mx + L->pad + (ICON_W + 2) * L->scale,
			     y + (L->nav_h - PIXFONT_H * L->scale) / 2,
			     L->scale, col, k_pages[i].name);
		y += L->nav_h;
	}
}

static void draw_picker(const struct layout *L)
{
	const struct item *it;
	int x, y, hx, cx, cy, hy, sw = 34 * L->k;
	int hyy, hw, hh;

	if (!g_pick_synced)
		sync_picker();
	it = &k_pages[g_page].items[g_sel];
	x = L->pk_x;
	y = L->pk_y;
	hx = x + SV_W + 12;
	cx = x + (int)(g_s * (SV_W - 1));
	cy = y + (int)((1.f - g_v) * (SV_H - 1));
	hy = y + (int)(g_h * (SV_H - 1));

	build_sv();
	build_bar();
	overlay_blit_rgba(x, y, SV_W, SV_H, g_sv);
	overlay_blit_rgba(hx, y, HUE_W, SV_H, g_bar);

	frame(x, y, SV_W, SV_H, C_GREEN, L->border);
	frame(hx, y, HUE_W, SV_H, C_GREEN, L->border);

	if (cx < x + 6)
		cx = x + 6;
	if (cx > x + SV_W - 7)
		cx = x + SV_W - 7;
	if (cy < y + 6)
		cy = y + 6;
	if (cy > y + SV_H - 7)
		cy = y + SV_H - 7;
	frame(cx - 6, cy - 6, 12, 12, 0x000000UL, 1);
	frame(cx - 5, cy - 5, 10, 10, 0xFFFFFFUL, 1);
	fill(hx - 3, hy - 2, HUE_W + 6, 5, 0x000000UL);
	fill(hx, hy - 1, HUE_W, 3, 0xFFFFFFUL);

	y += SV_H + L->pad;
	fill(x, y, L->pk_w, sw, *it->c);
	frame(x, y, L->pk_w, sw, C_GREEN, L->border);
	picker_hex_rect(L, &hx, &hyy, &hw, &hh);
	draw_hex(L, hx, hyy, hw, hh, *it->c, g_sel, C_GREEN);
}

static void draw_control(const struct layout *L, const struct page *p, int idx,
			 int ry, int rh, int rx1, bool sel)
{
	const struct item *it = &p->items[idx];
	char buf[16];

	if (it->kind == I_BOOL) {
		draw_switch(rx1 - L->sw_w, ry + (rh - L->sw_h) / 2, L->sw_w,
			    L->sw_h, *it->b);
	} else if (it->kind == I_COLOR) {
		int sw = 34 * L->k;
		int y = ry + (rh - sw) / 2;
		int hx, hy, hw, hh;

		fill(rx1 - sw, y, sw, sw, *it->c);
		frame(rx1 - sw, y, sw, sw, sel ? C_GREEN : C_GREY_D, L->border);
		row_hex_rect(L, idx, &hx, &hy, &hw, &hh);
		draw_hex(L, hx, hy, hw, hh, *it->c, idx,
			 sel ? C_GREEN : C_GREY);
	} else if (it->kind == I_KEY) {
		const char *n = overlay_key_name((unsigned long)*it->i);

		snprintf(buf, sizeof buf, "%s", n ? n : "-");
		pixfont_draw(rx1 - pixfont_width(buf, L->scale),
			     ry + (rh - PIXFONT_H * L->scale) / 2, L->scale,
			     g_capture == it->i ? C_PINK : C_GREEN, buf);
	} else {
		int pos = *it->i - it->lo;
		int n = it->hi - it->lo + 1;
		int x2 = rx1 - L->val_w;

		snprintf(buf, sizeof buf, "%d%s", *it->i, p->unit);
		draw_slider(x2 - L->sl_w, ry + (rh - L->sl_h) / 2, L->sl_w,
			    L->sl_h, pos, n);
		pixfont_draw(rx1 - pixfont_width(buf, L->scale),
			     ry + (rh - PIXFONT_H * L->scale) / 2, L->scale,
			     C_GREEN, buf);
	}
}

static void draw_content(const struct layout *L)
{
	const struct page *p = &k_pages[g_page];
	int rh = row_h_of(L, p->nitems);
	int vis = visible_rows(L, p->nitems);
	int y0 = L->cy + L->border + L->pad / 2;
	int rx1 = L->cx + L->cw - L->pad;
	int i, hov;

	overlay_fill_rgba(L->cx, L->cy, L->cw, L->ch, A_PANEL);
	frame(L->cx, L->cy, L->cw, L->ch, C_GREEN, L->border);
	if (p->picker)
		rx1 = L->pk_x - L->pad;
	hov = row_hit(L, g_mx, g_my);
	if (hov >= 0 && picker_hit(L, g_mx, g_my))
		hov = -1;

	for (i = g_scroll; i < p->nitems && i < g_scroll + vis; i++) {
		int ry = y0 + (i - g_scroll) * rh;
		bool sel = p->picker && i == g_sel;

		if (i == hov)
			overlay_fill_rgba(L->cx + L->border, ry,
					  L->cw - 2 * L->border, rh, A_HOVER);
		if (i != g_scroll)
			fill(L->cx + L->pad, ry, L->cw - 2 * L->pad, 1, C_HAIR);
		pixfont_draw(L->cx + L->pad,
			     ry + (rh - PIXFONT_H * L->scale) / 2, L->scale,
			     sel ? C_GREEN : C_TEXT, p->items[i].label);
		draw_control(L, p, i, ry, rh, rx1, sel);
	}
	if (p->picker)
		draw_picker(L);
}

void menu_draw(void)
{
	struct layout L;

	if (!g_open)
		return;
	layout(&L);
	overlay_pointer(&g_mx, &g_my, NULL, NULL);
	draw_scrim();
	draw_header(&L);
	draw_nav(&L);
	draw_content(&L);
	if (g_mouse_ours) {
		int s = 2 * L.k;

		sprite_draw(g_mx, g_my, s, k_cursor, N(k_cursor), 7, 'o',
			    0x000000UL);
		sprite_draw(g_mx, g_my, s, k_cursor, N(k_cursor), 7, 'x',
			    0xFFFFFFUL);
	}
}

static void slider_set(const struct layout *L, int idx, int mx)
{
	const struct item *it = &k_pages[g_page].items[idx];
	int w = L->sl_w - 2 * g_border;
	int x1 = L->cx + L->cw - L->pad - L->val_w - L->sl_w + g_border;
	int f = mx - x1;

	if (w <= 0)
		return;
	if (f < 0)
		f = 0;
	if (f > w)
		f = w;
	*it->i = it->lo + (int)((long)f * (it->hi - it->lo) / w);
	g_dirty = true;
}

static void sync_picker(void)
{
	const struct page *p = &k_pages[g_page];

	if (!p->picker)
		return;
	if (g_sel < 0 || g_sel >= p->nitems)
		g_sel = 0;
	rgb_to_hsv(*p->items[g_sel].c, &g_h, &g_s, &g_v);
	g_pick_synced = true;
}

static void picker_set(const struct layout *L, int x, int y)
{
	const struct item *it = &k_pages[g_page].items[g_sel];
	float f;

	if (g_pick == 2) {
		f = (float)(y - L->pk_y) / (float)(SV_H - 1);
		g_h = f < 0.f ? 0.f : (f > 1.f ? 1.f : f);
	} else if (g_pick == 1) {
		f = (float)(x - L->pk_x) / (float)(SV_W - 1);
		g_s = f < 0.f ? 0.f : (f > 1.f ? 1.f : f);
		f = 1.f - (float)(y - L->pk_y) / (float)(SV_H - 1);
		g_v = f < 0.f ? 0.f : (f > 1.f ? 1.f : f);
	} else {
		return;
	}
	*it->c = hsv_to_rgb(g_h, g_s, g_v) & 0xFFFFFFUL;
	g_dirty = true;
}

static void press(const struct layout *L, int x, int y)
{
	const struct page *p = &k_pages[g_page];
	const struct item *it;
	int i = nav_hit(L, x, y);

	if (i >= 0) {
		g_page = i;
		g_scroll = 0;
		g_drag = -1;
		sync_picker();
		return;
	}
	if (p->picker) {
		int hx, hy, hw, hh;

		picker_hex_rect(L, &hx, &hy, &hw, &hh);
		if (hit(x, y, hx, hy, hw, hh)) {
			start_edit(g_sel);
			return;
		}
		i = picker_hit(L, x, y);
		if (i) {
			g_pick = i;
			picker_set(L, x, y);
			return;
		}
	}
	for (i = 0; i < p->nitems; i++) {
		int hx, hy, hw, hh;

		if (p->items[i].kind != I_COLOR ||
		    i < g_scroll || i >= g_scroll + visible_rows(L, p->nitems))
			continue;
		row_hex_rect(L, i, &hx, &hy, &hw, &hh);
		if (hit(x, y, hx, hy, hw, hh)) {
			g_sel = i;
			sync_picker();
			start_edit(i);
			return;
		}
	}
	i = row_hit(L, x, y);
	if (i < 0)
		return;
	it = &p->items[i];
	if (it->kind == I_BOOL) {
		*it->b = !*it->b;
		config_save();
	} else if (it->kind == I_COLOR) {
		g_sel = i;
		sync_picker();
	} else if (it->kind == I_KEY) {
		g_capture = it->i;
	} else {
		g_drag = i;
		slider_set(L, i, x);
	}
}

static void scroll_by(const struct layout *L, int x, int y, int delta)
{
	const struct page *p = &k_pages[g_page];
	int vis, max;

	if (nav_hit(L, x, y) >= 0) {
		int pg = g_page + (delta < 0 ? -1 : 1);

		if (pg < 0 || pg >= (int)N(k_pages) || pg == g_page)
			return;
		g_page = pg;
		g_scroll = 0;
		sync_picker();
		return;
	}
	vis = visible_rows(L, p->nitems);
	max = p->nitems > vis ? p->nitems - vis : 0;
	g_scroll += delta < 0 ? -1 : 1;
	if (g_scroll < 0)
		g_scroll = 0;
	if (g_scroll > max)
		g_scroll = max;
}

static void release(void)
{
	g_drag = -1;
	g_pick = 0;
	if (g_dirty) {
		config_save();
		g_dirty = false;
	}
}

static void set_open(bool on)
{
	if (on == g_open)
		return;
	g_open = on;
	g_prev_btn = 0;
	g_drag = -1;
	g_pick = 0;
	g_edit = false;
	g_buf_n = 0;
	g_capture = NULL;
	overlay_input_grab(on);
	g_mouse_ours = overlay_mouse_grab(on) != 0;
	if (on) {
		sync_picker();
	} else if (g_dirty) {
		config_save();
		g_dirty = false;
	}
}

static int hex_digit(unsigned long ks)
{
	if (ks >= '0' && ks <= '9')
		return (int)(ks - '0');
	if (ks >= 'a' && ks <= 'f')
		return (int)(ks - 'a') + 10;
	if (ks >= 'A' && ks <= 'F')
		return (int)(ks - 'A') + 10;
	if (ks >= 0xFFB0UL && ks <= 0xFFB9UL)
		return (int)(ks - 0xFFB0UL);
	return -1;
}

static void start_edit(int row)
{
	g_edit = true;
	g_edit_sel = row;
	g_buf_n = 0;
	g_buf[0] = '\0';
}

static void commit_edit(void)
{
	const struct page *p = &k_pages[g_page];
	unsigned long v;
	char *end;

	if (!g_edit)
		return;
	g_edit = false;
	if (!g_buf_n || !p->picker || g_edit_sel < 0 || g_edit_sel >= p->nitems)
		return;
	v = strtoul(g_buf, &end, 16);
	if (*end)
		return;
	*p->items[g_edit_sel].c = v & 0xFFFFFFUL;
	sync_picker();
	config_save();
}

static void edit_key(unsigned long ks)
{
	int d = hex_digit(ks);

	if (ks == KEY_ESC) {
		g_edit = false;
		g_buf_n = 0;
	} else if (ks == KEY_ENTER || ks == KEY_KP_ENTER) {
		commit_edit();
	} else if (ks == KEY_BACKSPACE) {
		if (g_buf_n)
			g_buf[--g_buf_n] = '\0';
	} else if (d >= 0 && g_buf_n < 6) {
		g_buf[g_buf_n++] = "0123456789ABCDEF"[d];
		g_buf[g_buf_n] = '\0';
	}
}

static void edit_keys(void)
{
	unsigned long ks[16];
	int n = overlay_keys_pressed(ks, 16), i;

	for (i = 0; i < n; i++)
		edit_key(ks[i]);
}

void menu_init(void)
{
	int i;

	for (i = 0; i < CAT_H; i++)
		assert(strlen(k_cat[i]) == CAT_W);
	for (i = 0; i < ICON_H; i++)
		assert(strlen(k_icon_play[i]) == ICON_W &&
		       strlen(k_icon_eye[i]) == ICON_W &&
		       strlen(k_icon_x[i]) == ICON_W &&
		       strlen(k_icon_cross[i]) == ICON_W);
}

bool menu_is_open(void)
{
	return g_open;
}

static bool capture_key(void)
{
	unsigned long ks[8];
	int n = overlay_keys_pressed(ks, 8), i;

	for (i = 0; i < n; i++) {
		if (ks[i] == KEY_ESC) {
			g_capture = NULL;
			return true;
		}
		if (ks[i] == KEY_F11)
			continue;
		*g_capture = (int)ks[i];
		g_capture = NULL;
		config_save();
		return true;
	}
	return false;
}

void menu_update(void)
{
	struct layout L;
	unsigned btn = 0;
	int mx = 0, my = 0, wheel = 0;

	if (!g_open) {
		if (overlay_key_pressed(KEY_F11))
			set_open(true);
		return;
	}
	if (g_edit) {
		edit_keys();
		if (overlay_key_pressed(KEY_F11)) {
			commit_edit();
			set_open(false);
			return;
		}
	} else if (g_capture && capture_key()) {
		return;
	} else if (overlay_key_pressed(KEY_F11)) {
		set_open(false);
		return;
	} else if (overlay_key_pressed(KEY_ESC)) {
		set_open(false);
		return;
	}

	overlay_input_refocus();
	overlay_pointer(&mx, &my, &btn, &wheel);
	layout(&L);
	if (wheel)
		scroll_by(&L, mx, my, wheel);
	if (btn & 1) {
		if (!(g_prev_btn & 1)) {
			commit_edit();
			press(&L, mx, my);
		} else if (g_pick) {
			picker_set(&L, mx, my);
		} else if (g_drag >= 0) {
			slider_set(&L, g_drag, mx);
		}
	} else if (g_prev_btn & 1) {
		release();
	}
	g_prev_btn = btn;
}
