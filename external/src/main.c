#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "feature.h"
#include "game.h"
#include "mem.h"
#include "menu.h"
#include "offsets.h"
#include "overlay.h"

extern const struct feature esp_feature;
extern const struct feature triggerbot_feature;

static const struct feature *const g_features[] = {
	&esp_feature,
	&triggerbot_feature,
};

#define NFEATURES (sizeof g_features / sizeof g_features[0])

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

static void drop_privileges(void)
{
	const char *display = getenv("BLKCAT_DISPLAY");
	char path[64];
	struct stat st;
	size_t n;

	if (geteuid() != 0)
		return;
	if (!display || !*display)
		display = OVERLAY_DEFAULT_DISPLAY;
	n = strcspn(display + 1, ".");
	snprintf(path, sizeof path, "/tmp/.X11-unix/X%.*s", (int)n, display + 1);
	if (stat(path, &st) != 0) {
		fprintf(stderr, "blkcat: no display socket %s\n", path);
		return;
	}
	if (setgid(st.st_gid) != 0)
		return;
	if (setuid(st.st_uid) != 0)
		return;
	fprintf(stderr, "blkcat: dropped to uid %u for display access\n",
		(unsigned)st.st_uid);
}

int main(void)
{
	struct timespec next;
	size_t i;

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	config_load(NULL);
	if (cs2_offsets_load() != 0)
		return 1;
	if (game_attach() != 0)
		return 1;

	overlay_mouse_init();
	drop_privileges();
	if (overlay_init(g_game.width, g_game.height) != 0) {
		fprintf(stderr, "blkcat: overlay init failed\n");
		return 1;
	}

	g_game.width = overlay_width();
	g_game.height = overlay_height();
	menu_init();

	clock_gettime(CLOCK_MONOTONIC, &next);
	while (g_running) {
		config_reload_if_changed();
		overlay_keys_update();
		menu_update();
		game_update();
		for (i = 0; i < NFEATURES; i++)
			if (g_features[i]->tick)
				g_features[i]->tick();
		overlay_begin();
		for (i = 0; i < NFEATURES; i++)
			if (g_features[i]->draw)
				g_features[i]->draw();
		menu_draw();
		overlay_present();

		next.tv_nsec += 16666667L;
		if (next.tv_nsec >= 1000000000L) {
			next.tv_sec++;
			next.tv_nsec -= 1000000000L;
		}
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
	}

	for (i = 0; i < NFEATURES; i++)
		if (g_features[i]->stop)
			g_features[i]->stop();

	overlay_exit();
	mem_close();
	return 0;
}
