#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "cs2.h"

struct game {
	bool attached;
	pid_t pid;
	uintptr_t client;
	uintptr_t engine;
	uintptr_t ges;
	uintptr_t chunk0;
	bool ready;
	int local_team;
	uintptr_t local_pawn;
	float view[16];
	int width;
	int height;
};

extern struct game g_game;

int game_attach(void);
void game_update(void);
uintptr_t game_entity(int index);
bool w2s(const Vec3 *p, Vec2 *out);

#endif
