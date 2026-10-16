#ifndef FEATURE_H
#define FEATURE_H

struct feature {
	const char *name;
	void (*tick)(void);
	void (*draw)(void);
	void (*stop)(void);
};

#endif
