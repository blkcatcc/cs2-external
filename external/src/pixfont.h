#ifndef PIXFONT_H_
#define PIXFONT_H_

#define PIXFONT_W 5
#define PIXFONT_H 7
#define PIXFONT_ADV 6

void pixfont_draw(int x, int y, int scale, unsigned long rgb, const char *s);
int pixfont_width(const char *s, int scale);

#endif
