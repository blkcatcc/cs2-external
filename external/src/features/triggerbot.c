#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../../../kernel/blkcat_ioctl.h"
#include "../config.h"
#include "../cs2.h"
#include "../feature.h"
#include "../game.h"
#include "../mem.h"
#include "../menu.h"
#include "../overlay.h"
#include "../offsets.h"

static bool g_checked;
static bool g_armed;
static bool g_down;
static long long g_fire_at;
static long long g_release_at;
static int g_shots;
static int g_dump = -1;

static long long now_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int jittered(int base, int jitter, bool pct)
{
	int d = pct ? base * jitter / 100 : jitter;
	int v;

	if (d <= 0)
		return base;
	v = base + (int)(rand() % (2 * d + 1)) - d;
	return v < 0 ? 0 : v;
}

static void selfcheck(void)
{
	int i;

	assert(jittered(100, 0, true) == 100);
	assert(jittered(100, 0, false) == 100);
	for (i = 0; i < 64; i++) {
		int v = jittered(100, 25, true);

		assert(v >= 75 && v <= 125);
		v = jittered(100, 25, false);
		assert(v >= 75 && v <= 125);
		v = jittered(0, 40, false);
		assert(v >= 0 && v <= 40);
		v = jittered(10, 200, false);
		assert(v >= 0 && v <= 210);
	}
}

static void lift(void)
{
	if (g_down) {
		mem_mouse_button(BLKCAT_BTN_LEFT, 0);
		g_down = false;
	}
}

static void drop(void)
{
	lift();
	g_armed = false;
}

static bool on_target(void)
{
	int idx = mem_i32(g_game.local_pawn + g_off.m_i_id_ent_index);
	uintptr_t pawn;
	int team;

	if (idx <= 0)
		return false;
	pawn = game_entity(idx);
	if (!pawn || pawn == g_game.local_pawn)
		return false;
	if (mem_u8(pawn + g_off.m_life_state) != 0 ||
	    mem_i32(pawn + g_off.m_i_health) <= 0)
		return false;
	team = mem_u8(pawn + g_off.m_i_team_num);
	return team != g_game.local_team &&
	       (team == CS2_TEAM_T || team == CS2_TEAM_CT);
}

/* BLKCAT_DUMP_TRIGGER=1 or touch /tmp/blkcat_trigger prints why it does nothing */
static void debug_dump(void)
{
	static struct timespec last;
	struct timespec now;
	uintptr_t pawn = 0;
	int idx, life = -1, hp = -1, team = -1;

	if (g_dump < 0)
		g_dump = getenv("BLKCAT_DUMP_TRIGGER") != NULL ||
			  access("/tmp/blkcat_trigger", F_OK) == 0;
	if (!g_dump)
		return;
	clock_gettime(CLOCK_MONOTONIC, &now);
	if (last.tv_sec == now.tv_sec &&
	    now.tv_nsec - last.tv_nsec < 250000000L)
		return;
	last = now;

	idx = mem_i32(g_game.local_pawn + g_off.m_i_id_ent_index);
	if (idx > 0) {
		pawn = game_entity(idx);
		if (pawn) {
			life = mem_u8(pawn + g_off.m_life_state);
			hp = mem_i32(pawn + g_off.m_i_health);
			team = mem_u8(pawn + g_off.m_i_team_num);
		}
	}
	fprintf(stderr, "blkcat: trigger: on=%d ready=%d key=%d idx=%d pawn=%#lx"
		" life=%d hp=%d team=%d myteam=%d tgt=%d shots=%d\n",
		g_cfg.triggerbot, g_game.ready,
		overlay_key_down((unsigned long)g_cfg.trigger_hotkey), idx,
		(unsigned long)pawn, life, hp, team, g_game.local_team,
		on_target(), g_shots);
}

static void triggerbot_tick(void)
{
	long long now;

	if (!g_checked) {
		g_checked = true;
		srand((unsigned)time(NULL));
		selfcheck();
	}
	debug_dump();
	if (!g_cfg.triggerbot || !g_game.ready || menu_is_open()) {
		drop();
		return;
	}
	now = now_ms();
	if (!overlay_key_down((unsigned long)g_cfg.trigger_hotkey) ||
	    !on_target()) {
		drop();
		return;
	}
	if (!g_armed) {
		g_armed = true;
		g_fire_at = now + jittered(g_cfg.trigger_delay,
					   g_cfg.trigger_jitter,
					   g_cfg.trigger_jitter_pct);
	}
	if (g_down) {
		if (now >= g_release_at)
			lift();
		return;
	}
	if (now < g_fire_at)
		return;
	mem_mouse_button(BLKCAT_BTN_LEFT, 1);
	g_shots++;
	g_down = true;
	g_release_at = now + jittered(g_cfg.trigger_hold,
				      g_cfg.trigger_hold_jitter,
				      g_cfg.trigger_jitter_pct);
	g_fire_at = g_release_at + jittered(g_cfg.trigger_next,
					    g_cfg.trigger_jitter,
					    g_cfg.trigger_jitter_pct);
}

static void triggerbot_stop(void)
{
	lift();
}

const struct feature triggerbot_feature = {
	.name = "triggerbot",
	.tick = triggerbot_tick,
	.draw = NULL,
	.stop = triggerbot_stop,
};
