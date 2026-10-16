#ifndef OVERLAY_H
#define OVERLAY_H

#define OVERLAY_DEFAULT_DISPLAY ":1"

int overlay_init(int width, int height);

int overlay_width(void);
int overlay_height(void);
void overlay_begin(void);

void overlay_color(unsigned long rgb);
void overlay_line_width(int px);

void overlay_rect(int x, int y, int w, int h);
void overlay_fill_rect(int x, int y, int w, int h);
void overlay_line(int x0, int y0, int x1, int y1);
void overlay_circle(int cx, int cy, int r);

void overlay_text(int x, int y, const char *s);
void overlay_text_size(const char *s, int *w, int *h);

void overlay_fill_rgba(int x, int y, int w, int h, unsigned long argb);

void overlay_fill_rects(const int *xywh, int n);

void overlay_blit_rgba(int x, int y, int w, int h, const unsigned long *argb);

void overlay_keys_update(void);
int overlay_key_pressed(unsigned long keysym);

int overlay_key_down(unsigned long keysym);

const char *overlay_key_name(unsigned long keysym);

int overlay_keys_pressed(unsigned long *out, int max);

void overlay_input_grab(int on);

void overlay_mouse_init(void);

int overlay_mouse_grab(int on);

void overlay_input_refocus(void);

void overlay_pointer(int *x, int *y, unsigned *buttons, int *wheel);

void overlay_present(void);
void overlay_exit(void);

#endif
