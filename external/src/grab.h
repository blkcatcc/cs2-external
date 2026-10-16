#ifndef GRAB_H
#define GRAB_H

int grab_open(void);

int grab_begin(void);
void grab_end(void);

void grab_pump(int *dx, int *dy, unsigned *buttons, int *wheel);

#endif
