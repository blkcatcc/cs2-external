#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

struct config g_cfg = {
	.box = true,
	.full_box = false,
	.esp_thickness = 1,
	.esp_outline = true,
	.esp_outline_thickness = 1,
	.esp_skeleton = false,
	.name = true,
	.health_bar = true,
	.health_text = true,
	.weapon = true,
	.distance = true,
	.head_circle = false,

	.triggerbot = false,
	.trigger_hotkey = 32,
	.trigger_delay = 0,
	.trigger_jitter = 0,
	.trigger_jitter_pct = true,
	.trigger_hold = 30,
	.trigger_hold_jitter = 0,
	.trigger_next = 0,

	.box_color = 0xFF8000UL,
	.outline_color = 0x000000UL,
	.skeleton_color = 0xFFFFFFUL,
	.head_circle_color = 0xFF8000UL,
	.name_color = 0xFFFFFFUL,
	.health_color = 0x00FF00UL,
	.health_text_color = 0xFFFFFFUL,
	.weapon_color = 0xFFFFFFUL,
	.distance_color = 0xFFFFFFUL,
};

static const struct { const char *key; bool *p; } g_bools[] = {
	{ "box", &g_cfg.box },
	{ "full_box", &g_cfg.full_box },
	{ "esp_outline", &g_cfg.esp_outline },
	{ "esp_skeleton", &g_cfg.esp_skeleton },
	{ "name", &g_cfg.name },
	{ "health_bar", &g_cfg.health_bar },
	{ "health_text", &g_cfg.health_text },
	{ "weapon", &g_cfg.weapon },
	{ "distance", &g_cfg.distance },
	{ "head_circle", &g_cfg.head_circle },
	{ "triggerbot", &g_cfg.triggerbot },
	{ "trigger_jitter_pct", &g_cfg.trigger_jitter_pct },
};

static const struct { const char *key; int *p; int lo, hi; } g_ints[] = {
	{ "esp_thickness", &g_cfg.esp_thickness, 1, 8 },
	{ "esp_outline_thickness", &g_cfg.esp_outline_thickness, 1, 8 },
	{ "trigger_hotkey", &g_cfg.trigger_hotkey, 0, 0x7FFFFFFF },
	{ "trigger_delay", &g_cfg.trigger_delay, 0, 1000 },
	{ "trigger_jitter", &g_cfg.trigger_jitter, 0, 100 },
	{ "trigger_hold", &g_cfg.trigger_hold, 1, 500 },
	{ "trigger_hold_jitter", &g_cfg.trigger_hold_jitter, 0, 100 },
	{ "trigger_next", &g_cfg.trigger_next, 0, 1000 },
};

static const struct { const char *key; unsigned long *p; } g_colors[] = {
	{ "box_color", &g_cfg.box_color },
	{ "outline_color", &g_cfg.outline_color },
	{ "skeleton_color", &g_cfg.skeleton_color },
	{ "head_circle_color", &g_cfg.head_circle_color },
	{ "name_color", &g_cfg.name_color },
	{ "health_color", &g_cfg.health_color },
	{ "health_text_color", &g_cfg.health_text_color },
	{ "weapon_color", &g_cfg.weapon_color },
	{ "distance_color", &g_cfg.distance_color },
};

#define N(a) (sizeof (a) / sizeof (a)[0])

static char g_path[256];
static struct timespec g_mtime;
static off_t g_size;
static bool g_loaded;

static char *trim(char *s)
{
	char *end;

	while (*s && isspace((unsigned char)*s))
		s++;
	end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1]))
		*--end = '\0';
	return s;
}

static bool parse_bool(const char *v, bool *out)
{
	if (!strcmp(v, "true") || !strcmp(v, "1") || !strcmp(v, "on") ||
	    !strcmp(v, "yes")) {
		*out = true;
		return true;
	}
	if (!strcmp(v, "false") || !strcmp(v, "0") || !strcmp(v, "off") ||
	    !strcmp(v, "no")) {
		*out = false;
		return true;
	}
	return false;
}

static bool parse_color(const char *v, unsigned long *out)
{
	char *end;
	unsigned long c;

	if (*v == '#')
		v++;
	if (!isxdigit((unsigned char)*v))
		return false;
	c = strtoul(v, &end, 16);
	if (*end || c > 0xFFFFFFUL)
		return false;
	*out = c;
	return true;
}

static void apply(const char *key, const char *val)
{
	size_t i;

	for (i = 0; i < N(g_bools); i++)
		if (!strcmp(key, g_bools[i].key)) {
			if (!parse_bool(val, g_bools[i].p))
				fprintf(stderr, "blkcat: config: %s: bad boolean '%s'\n",
					key, val);
			return;
		}
	for (i = 0; i < N(g_ints); i++)
		if (!strcmp(key, g_ints[i].key)) {
			char *end;
			long v = strtol(val, &end, 10);

			if (end == val || *end) {
				fprintf(stderr, "blkcat: config: %s: bad integer '%s'\n",
					key, val);
				return;
			}
			if (v < g_ints[i].lo)
				v = g_ints[i].lo;
			if (v > g_ints[i].hi)
				v = g_ints[i].hi;
			*g_ints[i].p = (int)v;
			return;
		}
	for (i = 0; i < N(g_colors); i++)
		if (!strcmp(key, g_colors[i].key)) {
			if (!parse_color(val, g_colors[i].p))
				fprintf(stderr, "blkcat: config: %s: bad colour '%s'\n",
					key, val);
			return;
		}
	fprintf(stderr, "blkcat: config: unknown key '%s'\n", key);
}

void config_load(const char *path)
{
	struct stat st;
	char line[256];
	FILE *f;

	if (!path || !*path) {
		path = getenv("BLKCAT_CONFIG");
		if (!path || !*path)
			path = "blkcat.cfg";
	}

	if (path != g_path)
		snprintf(g_path, sizeof g_path, "%s", path);

	if (stat(g_path, &st) != 0) {
		fprintf(stderr, "blkcat: config: %s missing, using defaults\n",
			g_path);
		g_loaded = false;
		return;
	}
	f = fopen(g_path, "r");
	if (!f) {
		fprintf(stderr, "blkcat: config: cannot read %s\n", g_path);
		g_loaded = false;
		return;
	}
	while (fgets(line, sizeof line, f)) {
		char *eq, *hash, *k, *v;

		hash = strstr(line, " #");
		while (hash && hash[2] && !isspace((unsigned char)hash[2]))
			hash = strstr(hash + 1, " #");
		if (hash)
			*hash = '\0';
		eq = strchr(line, '=');
		if (!eq)
			continue;
		*eq = '\0';
		k = trim(line);
		v = trim(eq + 1);
		if (!*k || *k == '#')
			continue;
		apply(k, v);
	}
	fclose(f);
	g_mtime = st.st_mtim;
	g_size = st.st_size;
	g_loaded = true;
	fprintf(stderr, "blkcat: config loaded from %s\n", g_path);
}

void config_reload_if_changed(void)
{
	struct stat st;

	if (!g_path[0] || stat(g_path, &st) != 0)
		return;
	if (g_loaded && st.st_mtim.tv_sec == g_mtime.tv_sec &&
	    st.st_mtim.tv_nsec == g_mtime.tv_nsec && st.st_size == g_size)
		return;
	config_load(g_path);
}

#define NBOOLS ((int)N(g_bools))
#define NINTS  ((int)N(g_ints))
#define NCOLS  ((int)N(g_colors))
#define NKEYS  (NBOOLS + NINTS + NCOLS)

static const char *key_name(int i)
{
	if (i < NBOOLS)
		return g_bools[i].key;
	i -= NBOOLS;
	if (i < NINTS)
		return g_ints[i].key;
	return g_colors[i - NINTS].key;
}

static void write_value(FILE *f, int i)
{
	if (i < NBOOLS)
		fprintf(f, "%s", *g_bools[i].p ? "true" : "false");
	else if (i - NBOOLS < NINTS)
		fprintf(f, "%d", *g_ints[i - NBOOLS].p);
	else
		fprintf(f, "%06lX", *g_colors[i - NBOOLS - NINTS].p);
}

static int key_index(const char *k)
{
	int i;

	for (i = 0; i < NKEYS; i++)
		if (!strcmp(k, key_name(i)))
			return i;
	return -1;
}

void config_save(void)
{
	char line[256], *txt = NULL;
	bool seen[NKEYS];
	size_t len = 0, cap = 0, i;
	struct stat st;
	FILE *f, *out;

	if (!g_path[0])
		return;
	memset(seen, 0, sizeof seen);
	f = fopen(g_path, "r");
	if (f) {
		while (fgets(line, sizeof line, f)) {
			size_t n = strlen(line);

			if (len + n + 1 > cap) {
				char *nb;

				cap = (len + n + 1) * 2;
				nb = realloc(txt, cap);
				if (!nb) {
					fclose(f);
					free(txt);
					return;
				}
				txt = nb;
			}
			memcpy(txt + len, line, n + 1);
			len += n;
		}
		fclose(f);
	}
	out = fopen(g_path, "w");
	if (!out) {
		fprintf(stderr, "blkcat: config: cannot write %s\n", g_path);
		free(txt);
		return;
	}
	for (i = 0; txt && txt[i]; ) {
		char *p = txt + i;
		char *nl = strchr(p, '\n');
		size_t n = nl ? (size_t)(nl - p) : strlen(p);
		char *eq = memchr(p, '=', n);
		int idx = -1;

		if (eq) {
			char key[64];
			size_t a = 0, b = (size_t)(eq - p);

			while (a < b && isspace((unsigned char)p[a]))
				a++;
			while (b > a && isspace((unsigned char)p[b - 1]))
				b--;
			if (b - a < sizeof key) {
				memcpy(key, p + a, b - a);
				key[b - a] = '\0';
				idx = key_index(key);
			}
		}
		if (idx >= 0) {
			fwrite(p, 1, (size_t)(eq - p) + 1, out);
			fputc(' ', out);
			write_value(out, idx);
			fputc('\n', out);
			seen[idx] = true;
		} else {
			fwrite(p, 1, n, out);
			fputc('\n', out);
		}
		i += n;
		if (nl)
			i++;
	}
	for (i = 0; i < NKEYS; i++) {
		if (seen[i])
			continue;
		fprintf(out, "%s = ", key_name((int)i));
		write_value(out, (int)i);
		fputc('\n', out);
	}
	fclose(out);
	free(txt);

	if (stat(g_path, &st) == 0) {
		g_mtime = st.st_mtim;
		g_size = st.st_size;
	}
	g_loaded = true;
}
