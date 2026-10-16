CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=gnu11
CFLAGS += -MMD -MP
CPPFLAGS += $(shell pkg-config --cflags libcurl)
LDLIBS := -lX11 -lXext -lXrender -lm $(shell pkg-config --libs libcurl)

SRC := $(wildcard src/*.c src/features/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(OBJ:.o=.d)

blkcat: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

run: blkcat
	sudo ./blkcat

clean:
	rm -f $(OBJ) $(DEP) blkcat

-include $(DEP)

.PHONY: run clean
