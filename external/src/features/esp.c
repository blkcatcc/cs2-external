#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../cs2.h"
#include "../config.h"
#include "../feature.h"
#include "../game.h"
#include "../mem.h"
#include "../overlay.h"

enum {
	BONE_PELVIS = 1,
	BONE_SPINE_1 = 3,
	BONE_SPINE_2 = 4,
	BONE_NECK = 6,
	BONE_HEAD = 7,
	BONE_SHOULDER_L = 9,
	BONE_ELBOW_L = 10,
	BONE_HAND_L = 11,
	BONE_SHOULDER_R = 13,
	BONE_ELBOW_R = 14,
	BONE_HAND_R = 15,
	BONE_HIP_L = 17,
	BONE_KNEE_L = 18,
	BONE_FOOT_L = 19,
	BONE_HIP_R = 20,
	BONE_KNEE_R = 21,
	BONE_FOOT_R = 22,
	BONE_CHEST = 23,
};

static const int g_skel[][2] = {
	{ BONE_PELVIS, BONE_SPINE_1 }, { BONE_SPINE_1, BONE_SPINE_2 },
	{ BONE_SPINE_2, BONE_CHEST }, { BONE_CHEST, BONE_NECK },
	{ BONE_NECK, BONE_SHOULDER_L }, { BONE_SHOULDER_L, BONE_ELBOW_L },
	{ BONE_ELBOW_L, BONE_HAND_L },
	{ BONE_NECK, BONE_SHOULDER_R }, { BONE_SHOULDER_R, BONE_ELBOW_R },
	{ BONE_ELBOW_R, BONE_HAND_R },
	{ BONE_PELVIS, BONE_HIP_L }, { BONE_HIP_L, BONE_KNEE_L },
	{ BONE_KNEE_L, BONE_FOOT_L },
	{ BONE_PELVIS, BONE_HIP_R }, { BONE_HIP_R, BONE_KNEE_R },
	{ BONE_KNEE_R, BONE_FOOT_R },
};

struct bone {
	Vec3 pos;
	float pad[5];
};

_Static_assert(sizeof(struct bone) == CS2_BONE_STRIDE, "bone stride mismatch");
_Static_assert(BONE_CHEST < CS2_BONE_COUNT, "bone index out of range");

static const struct { unsigned int id; const char *name; } g_weapons[] = {
	{ 1, "Desert Eagle" }, { 2, "Dual Berettas" }, { 3, "Five-SeveN" },
	{ 4, "Glock-18" }, { 7, "AK-47" }, { 8, "AUG" }, { 9, "AWP" },
	{ 10, "FAMAS" }, { 11, "G3SG1" }, { 13, "Galil AR" }, { 14, "M249" },
	{ 16, "M4A4" }, { 17, "MAC-10" }, { 19, "P90" }, { 23, "MP5-SD" },
	{ 24, "UMP-45" }, { 25, "XM1014" }, { 26, "PP-Bizon" }, { 27, "MAG-7" },
	{ 28, "Negev" }, { 29, "Sawed-Off" }, { 30, "Tec-9" }, { 31, "Zeus x27" },
	{ 32, "P2000" }, { 33, "MP7" }, { 34, "MP9" }, { 35, "Nova" },
	{ 36, "P250" }, { 38, "SCAR-20" }, { 39, "SG 553" }, { 40, "SSG 08" },
	{ 43, "Flashbang" }, { 44, "HE Grenade" }, { 45, "Smoke Grenade" },
	{ 46, "Molotov" }, { 47, "Decoy" }, { 48, "Incendiary" }, { 49, "C4" },
	{ 59, "Knife" }, { 60, "M4A1-S" }, { 61, "USP-S" }, { 63, "CZ75-Auto" },
	{ 64, "R8 Revolver" },
	{ 500, "Bayonet" }, { 505, "Flip Knife" }, { 506, "Gut Knife" },
	{ 507, "Karambit" }, { 508, "M9 Bayonet" }, { 509, "Huntsman Knife" },
	{ 512, "Falchion Knife" }, { 514, "Bowie Knife" },
	{ 515, "Butterfly Knife" }, { 516, "Shadow Daggers" },
	{ 517, "Ursus Knife" }, { 518, "Navaja Knife" }, { 519, "Stiletto Knife" },
	{ 520, "Talon Knife" }, { 521, "Classic Knife" },
	{ 522, "Paracord Knife" }, { 523, "Survival Knife" }, { 524, "Nomad Knife" },
	{ 525, "Skeleton Knife" }, { 526, "Kukri Knife" },
};

#define N(a) (sizeof (a) / sizeof (a)[0])

#define CS2_UNIT_METRES 0.01905f

#define CS2_HEAD_DROP   6.0f
#define CS2_HEAD_RADIUS 4.5f

static void esp_line(int x0, int y0, int x1, int y1, unsigned long col)
{
	if (g_cfg.esp_outline) {
		overlay_color(g_cfg.outline_color);
		overlay_line_width(g_cfg.esp_thickness +
				   2 * g_cfg.esp_outline_thickness);
		overlay_line(x0, y0, x1, y1);
	}
	overlay_color(col);
	overlay_line_width(g_cfg.esp_thickness);
	overlay_line(x0, y0, x1, y1);
}

static void esp_rect(int x, int y, int w, int h, unsigned long col)
{
	if (g_cfg.esp_outline) {
		overlay_color(g_cfg.outline_color);
		overlay_line_width(g_cfg.esp_thickness +
				   2 * g_cfg.esp_outline_thickness);
		overlay_rect(x, y, w, h);
	}
	overlay_color(col);
	overlay_line_width(g_cfg.esp_thickness);
	overlay_rect(x, y, w, h);
}

static void esp_corners(int x, int y, int w, int h, unsigned long col)
{
	int ax = w / 4, ay = h / 4;

	if (ax < 2)
		ax = 2;
	if (ay < 2)
		ay = 2;
	esp_line(x, y, x + ax, y, col);
	esp_line(x, y, x, y + ay, col);
	esp_line(x + w, y, x + w - ax, y, col);
	esp_line(x + w, y, x + w, y + ay, col);
	esp_line(x, y + h, x + ax, y + h, col);
	esp_line(x, y + h, x, y + h - ay, col);
	esp_line(x + w, y + h, x + w - ax, y + h, col);
	esp_line(x + w, y + h, x + w, y + h - ay, col);
}

static void esp_circle(int cx, int cy, int r, unsigned long col)
{
	if (g_cfg.esp_outline) {
		overlay_color(g_cfg.outline_color);
		overlay_line_width(g_cfg.esp_thickness +
				   2 * g_cfg.esp_outline_thickness);
		overlay_circle(cx, cy, r);
	}
	overlay_color(col);
	overlay_line_width(g_cfg.esp_thickness);
	overlay_circle(cx, cy, r);
}

static void esp_text(int x, int y, const char *s, unsigned long col)
{
	int t = g_cfg.esp_outline ? g_cfg.esp_outline_thickness : 0;

	if (t) {
		overlay_color(g_cfg.outline_color);
		overlay_text(x - t, y, s);
		overlay_text(x + t, y, s);
		overlay_text(x, y - t, s);
		overlay_text(x, y + t, s);
	}
	overlay_color(col);
	overlay_text(x, y, s);
}

static void esp_text_centered(int cx, int y, const char *s, unsigned long col)
{
	int w, h;

	overlay_text_size(s, &w, &h);
	esp_text(cx - w / 2, y, s, col);
}

static int line_height(void)
{
	int w, h;

	overlay_text_size("X", &w, &h);
	return h;
}

static const char *weapon_name(unsigned int id)
{
	size_t i;

	for (i = 0; i < N(g_weapons); i++)
		if (g_weapons[i].id == id)
			return g_weapons[i].name;
	return NULL;
}

static unsigned long health_colour(int hp)
{
	int t = hp < 0 ? 0 : (hp > 100 ? 100 : hp);
	int r = 255 - (255 - (int)((g_cfg.health_color >> 16) & 0xFF)) * t / 100;
	int g = (int)((g_cfg.health_color >> 8) & 0xFF) * t / 100;
	int b = (int)(g_cfg.health_color & 0xFF) * t / 100;

	return (unsigned long)((r << 16) | (g << 8) | b);
}

static void read_name(uintptr_t ctrl, char *out, size_t n)
{
	char buf[128];
	size_t len;

	out[0] = '\0';
	if (!g_off.m_isz_player_name ||
	    !mem_read_raw(ctrl + g_off.m_isz_player_name, buf, sizeof buf))
		return;
	buf[sizeof buf - 1] = '\0';
	len = strcspn(buf, "\r\n");
	if (len >= n)
		len = n - 1;
	memcpy(out, buf, len);
	out[len] = '\0';
}

static void read_weapon(uintptr_t pawn, char *out, size_t n)
{
	const char *name;
	uintptr_t services, wpn;
	uint16_t def;

	out[0] = '\0';
	if (!g_off.m_p_weapon_services || !g_off.m_h_active_weapon ||
	    !g_off.m_attribute_manager || !g_off.m_item ||
	    !g_off.m_item_definition_index)
		return;
	services = mem_u64(pawn + g_off.m_p_weapon_services);
	if (!services)
		return;
	wpn = game_entity((int)(mem_u32(services + g_off.m_h_active_weapon) &
				CS2_HANDLE_MASK));
	if (!wpn)
		return;
	if (!mem_read_raw(wpn + g_off.m_attribute_manager + g_off.m_item +
			  g_off.m_item_definition_index, &def, sizeof def))
		return;
	name = weapon_name(def);
	if (name)
		snprintf(out, n, "%s", name);
}

static bool bone_on_body(const Vec2 *p, int x, int y, int w, int h)
{
	return p->x >= x - w && p->x <= x + 2 * w &&
	       p->y >= y - h / 4 && p->y <= y + h + h / 4;
}

static bool read_bones(uintptr_t node, struct bone *out)
{
	uintptr_t arr;

	if (!g_off.m_model_state)
		return false;
	arr = mem_u64(node + g_off.m_model_state + CS2_BONE_ARRAY_OFF);
	if (!arr)
		return false;
	return mem_read_raw(arr, out, CS2_BONE_COUNT * sizeof *out);
}

static int g_dump = -1;
static bool g_dumped;

static void dump_bones(uintptr_t node, const Vec3 *feet, const struct bone *b)
{
	uintptr_t ms = node + g_off.m_model_state;
	uint64_t words[0x20];
	FILE *f = fopen("/tmp/blkcat_bones.txt", "w");
	int i;

	if (!f)
		return;
	fprintf(f, "feet %.2f %.2f %.2f  sceneNode %#lx  m_model_state +%#lx\n",
		feet->x, feet->y, feet->z, (unsigned long)node,
		(unsigned long)g_off.m_model_state);
	if (mem_read_raw(ms, words, sizeof words)) {
		fprintf(f, "\nCModelState @ %#lx\n", (unsigned long)ms);
		for (i = 0; i < 0x20; i++)
			fprintf(f, "  +%03x  %#018lx\n", i * 8,
				(unsigned long)words[i]);
		fprintf(f, "\npointers in CModelState -> first 3 entries\n");
		for (i = 0; i < 0x20; i++) {
			struct bone t[3];
			uintptr_t p = words[i];

			if (p < 0x100000000UL || p > 0x800000000000UL)
				continue;
			if (!mem_read_raw(p, t, sizeof t))
				continue;
			fprintf(f, "  [+%03x] %#lx  (%.1f %.1f %.1f) (%.1f %.1f %.1f) (%.1f %.1f %.1f)\n",
				i * 8, (unsigned long)p,
				t[0].pos.x, t[0].pos.y, t[0].pos.z,
				t[1].pos.x, t[1].pos.y, t[1].pos.z,
				t[2].pos.x, t[2].pos.y, t[2].pos.z);
		}
	}
	fprintf(f, "\nbones at m_model_state + 0x%x\n", CS2_BONE_ARRAY_OFF);
	for (i = 0; i < CS2_BONE_COUNT; i++) {
		float dx = b[i].pos.x - feet->x;
		float dy = b[i].pos.y - feet->y;
		float dz = b[i].pos.z - feet->z;

		fprintf(f, "  %2d %9.2f %9.2f %9.2f   d %7.2f dz %7.2f\n",
			i, b[i].pos.x, b[i].pos.y, b[i].pos.z,
			sqrtf(dx * dx + dy * dy + dz * dz), dz);
	}
	fclose(f);
	fprintf(stderr, "blkcat: bone dump written to /tmp/blkcat_bones.txt\n");
}

static Vec3 head_centre(const Vec3 *feet, const Vec3 *top,
			const struct bone *bones, const bool *ok)
{
	if (ok[BONE_HEAD])
		return bones[BONE_HEAD].pos;
	return (Vec3){ feet->x, feet->y, top->z - CS2_HEAD_DROP };
}

static void esp_draw(void)
{
	struct bone bones[CS2_BONE_COUNT];
	Vec2 bs[CS2_BONE_COUNT];
	bool ok[CS2_BONE_COUNT];
	Vec3 local;
	bool have_local = false;
	int i, lh;

	if (!g_game.ready)
		return;
	lh = line_height();

	if (g_game.local_pawn) {
		uintptr_t ln = mem_u64(g_game.local_pawn + g_off.m_p_game_scene_node);

		if (ln)
			have_local = mem_read_raw(ln + g_off.m_vec_abs_origin,
						  &local, sizeof local);
	}

	for (i = 1; i <= CS2_MAX_PLAYERS; i++) {
		uintptr_t ctrl = game_entity(i);
		uintptr_t pawn, node;
		uint32_t handle;
		Vec3 feet, head;
		float view[3];
		Vec2 sf, sh, hc, ht;
		int team, hp, j, x, y, bw, bh, ly;
		float h, w;
		char name[64], weapon[32], text[32];
		bool have_bones = false, have_head = false;

		if (!ctrl)
			continue;
		handle = mem_u32(ctrl + g_off.m_h_player_pawn);
		if (!handle)
			continue;
		pawn = game_entity((int)(handle & CS2_HANDLE_MASK));
		if (!pawn)
			continue;
		if (mem_u8(pawn + g_off.m_life_state) != 0)
			continue;
		hp = mem_i32(pawn + g_off.m_i_health);
		if (hp <= 0)
			continue;
		team = mem_u8(pawn + g_off.m_i_team_num);
		if (team != CS2_TEAM_T && team != CS2_TEAM_CT)
			continue;
		if (team == g_game.local_team)
			continue;
		node = mem_u64(pawn + g_off.m_p_game_scene_node);
		if (!node)
			continue;
		if (!mem_read_raw(node + g_off.m_vec_abs_origin, &feet,
				  sizeof feet))
			continue;
		if (!mem_read_raw(pawn + g_off.m_vec_view_offset, view,
				  sizeof view))
			continue;
		head = feet;

		head.z += view[2] + 8.0f;
		if (!w2s(&feet, &sf) || !w2s(&head, &sh))
			continue;
		h = sf.y - sh.y;
		if (h < 4.0f)
			continue;
		w = h * 0.5f;

		x = (int)(sf.x - w * 0.5f);
		y = (int)sh.y;
		bw = (int)w;
		bh = (int)h;

		if (g_cfg.esp_skeleton || g_cfg.head_circle) {
			memset(ok, 0, sizeof ok);
			have_bones = read_bones(node, bones);
			if (have_bones) {
				for (j = 0; j < CS2_BONE_COUNT; j++)
					ok[j] = w2s(&bones[j].pos, &bs[j]) &&
						bone_on_body(&bs[j], x, y, bw, bh);
				if (g_dump < 0)
					g_dump = getenv("BLKCAT_DUMP_BONES") != NULL ||
						 access("/tmp/blkcat_dump", F_OK) == 0;
				if (g_dump && !g_dumped) {
					g_dumped = true;
					dump_bones(node, &feet, bones);
				}
			}
		}

		if (g_cfg.head_circle || g_cfg.esp_skeleton) {
			Vec3 c = head_centre(&feet, &head, bones, ok);
			Vec3 t = { c.x, c.y, c.z + CS2_HEAD_RADIUS };

			have_head = w2s(&c, &hc) && w2s(&t, &ht);
		}

		if (g_cfg.full_box)
			esp_rect(x, y, bw, bh, g_cfg.box_color);
		if (g_cfg.box)
			esp_corners(x, y, bw, bh, g_cfg.box_color);

		if (g_cfg.esp_skeleton && have_bones) {
			size_t k;

			for (k = 0; k < N(g_skel); k++) {
				int a = g_skel[k][0], b = g_skel[k][1];

				if (ok[a] && ok[b])
					esp_line((int)bs[a].x, (int)bs[a].y,
						 (int)bs[b].x, (int)bs[b].y,
						 g_cfg.skeleton_color);
			}
			if (have_head && ok[BONE_NECK])
				esp_line((int)hc.x, (int)hc.y,
					 (int)bs[BONE_NECK].x,
					 (int)bs[BONE_NECK].y,
					 g_cfg.skeleton_color);
		}

		if (g_cfg.head_circle && have_head) {
			int r = (int)(hc.y - ht.y);

			if (r < 2)
				r = 2;
			esp_circle((int)hc.x, (int)hc.y, r,
				   g_cfg.head_circle_color);
		}

		if (g_cfg.health_bar) {
			int bx = x - 6;
			int fill = bh * (hp > 100 ? 100 : hp) / 100;

			overlay_color(g_cfg.outline_color);
			overlay_fill_rect(bx - 1, y - 1, 5, bh + 2);
			overlay_color(health_colour(hp));
			overlay_fill_rect(bx, y + bh - fill, 3, fill);
		}
		if (g_cfg.health_text) {
			int tw, th;

			snprintf(text, sizeof text, "%d", hp);
			overlay_text_size(text, &tw, &th);
			esp_text(x - 9 - tw, y + bh / 2 + th / 2, text,
				 g_cfg.health_text_color);
		}

		if (g_cfg.name) {
			read_name(ctrl, name, sizeof name);
			if (name[0])
				esp_text_centered(x + bw / 2, y - 4, name,
						  g_cfg.name_color);
		}

		ly = y + bh + 2;
		if (g_cfg.weapon) {
			read_weapon(pawn, weapon, sizeof weapon);
			if (weapon[0]) {
				esp_text_centered(x + bw / 2, ly + lh, weapon,
						  g_cfg.weapon_color);
				ly += lh;
			}
		}
		if (g_cfg.distance && have_local) {
			float dx = feet.x - local.x;
			float dy = feet.y - local.y;
			float dz = feet.z - local.z;
			float dist = sqrtf(dx * dx + dy * dy + dz * dz) *
				     CS2_UNIT_METRES;

			snprintf(text, sizeof text, "%.0fm", dist);
			esp_text_centered(x + bw / 2, ly + lh, text,
					  g_cfg.distance_color);
		}
	}
}

const struct feature esp_feature = {
	.name = "esp",
	.tick = NULL,
	.draw = esp_draw,
};
