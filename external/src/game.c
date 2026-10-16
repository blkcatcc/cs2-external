#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "mem.h"

struct game g_game;

static pid_t find_cs2(void)
{
	DIR *d = opendir("/proc");
	struct dirent *e;
	pid_t found = -1;

	if (!d)
		return -1;
	while ((e = readdir(d))) {
		char path[288], comm[32] = { 0 };
		FILE *f;

		if (!isdigit((unsigned char)e->d_name[0]))
			continue;
		snprintf(path, sizeof path, "/proc/%s/comm", e->d_name);
		f = fopen(path, "r");
		if (!f)
			continue;
		if (fgets(comm, sizeof comm, f)) {
			comm[strcspn(comm, "\n")] = '\0';
			if (!strcmp(comm, "cs2")) {
				found = (pid_t)atoi(e->d_name);
				fclose(f);
				break;
			}
		}
		fclose(f);
	}
	closedir(d);
	return found;
}

static uintptr_t module_base(pid_t pid, const char *name)
{
	char path[64], line[512];
	FILE *f;
	uintptr_t base = 0;

	snprintf(path, sizeof path, "/proc/%d/maps", pid);
	f = fopen(path, "r");
	if (!f)
		return 0;
	while (fgets(line, sizeof line, f)) {
		if (!strstr(line, name))
			continue;
		base = (uintptr_t)strtoul(line, NULL, 16);
		break;
	}
	fclose(f);
	return base;
}

int game_attach(void)
{
	pid_t pid = find_cs2();

	if (pid <= 0) {
		fprintf(stderr, "blkcat: cs2 not running\n");
		return -1;
	}
	g_game.client = module_base(pid, "libclient.so");
	g_game.engine = module_base(pid, "libengine2.so");
	if (!g_game.client || !g_game.engine) {
		fprintf(stderr, "blkcat: game modules not found\n");
		return -1;
	}
	if (mem_open(pid) != 0) {
		perror("blkcat: /dev/blkcat");
		return -1;
	}
	g_game.pid = pid;
	g_game.width = mem_i32(g_game.engine + g_off.dw_window_width);
	g_game.height = mem_i32(g_game.engine + g_off.dw_window_height);
	if (g_game.width <= 0 || g_game.height <= 0) {
		g_game.width = 1280;
		g_game.height = 1024;
	}
	g_game.attached = true;
	fprintf(stderr, "blkcat: pid %d client %#lx engine %#lx screen %dx%d\n",
		(int)pid, (unsigned long)g_game.client,
		(unsigned long)g_game.engine, g_game.width, g_game.height);
	return 0;
}

uintptr_t game_entity(int index)
{
	uintptr_t chunk;

	if (index < 0 || !g_game.ges)
		return 0;
	if ((index >> CS2_ENT_CHUNK_SHIFT) == 0)
		chunk = g_game.chunk0;
	else
		chunk = mem_u64(g_game.ges + 8 * (index >> CS2_ENT_CHUNK_SHIFT) +
				CS2_ENT_CHUNK_BASE);
	if (!chunk)
		return 0;
	return mem_u64(chunk + CS2_ENT_STRIDE * (index & CS2_ENT_CHUNK_MASK));
}

void game_update(void)
{
	uintptr_t ctrl;
	uint32_t handle;

	g_game.ready = false;
	if (!g_game.attached)
		return;
	ctrl = mem_u64(g_game.client + g_off.dw_local_player_controller);
	if (!ctrl)
		return;
	g_game.ges = mem_u64(g_game.client + g_off.dw_game_entity_system);
	g_game.chunk0 = g_game.ges ? mem_u64(g_game.ges + CS2_ENT_CHUNK_BASE) : 0;
	g_game.local_team = mem_u8(ctrl + g_off.m_i_team_num);
	handle = mem_u32(ctrl + g_off.m_h_player_pawn);
	g_game.local_pawn = handle ? game_entity((int)(handle & CS2_HANDLE_MASK)) : 0;
	mem_read_raw(g_game.client + g_off.dw_view_matrix, g_game.view,
		     sizeof g_game.view);
	g_game.ready = g_game.local_pawn != 0;
}

bool w2s(const Vec3 *p, Vec2 *out)
{
	const float *m = g_game.view;
	float w = m[12] * p->x + m[13] * p->y + m[14] * p->z + m[15];
	float sx, sy;

	if (w < 0.01f)
		return false;
	sx = (m[0] * p->x + m[1] * p->y + m[2] * p->z + m[3]) / w;
	sy = (m[4] * p->x + m[5] * p->y + m[6] * p->z + m[7]) / w;
	out->x = (sx + 1.0f) * 0.5f * (float)g_game.width;
	out->y = (1.0f - (sy + 1.0f) * 0.5f) * (float)g_game.height;
	return true;
}
