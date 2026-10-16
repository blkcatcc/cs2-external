#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "cs2.h"
#include "offsets.h"

#define OFFSETS_URL \
	"https://raw.githubusercontent.com/blkcatcc/cs2-dumper/main/output/offsets.json"
#define SCHEMA_URL \
	"https://raw.githubusercontent.com/blkcatcc/cs2-dumper/main/output/libclient_so.json"

struct cs2_offsets g_off;

struct buf {
	char *data;
	size_t len;
};

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *ud)
{
	struct buf *b = ud;
	size_t n = size * nmemb;
	char *p = realloc(b->data, b->len + n + 1);

	if (!p)
		return 0;
	b->data = p;
	memcpy(b->data + b->len, ptr, n);
	b->len += n;
	b->data[b->len] = '\0';
	return n;
}

static char *http_get(const char *url)
{
	struct buf b = { NULL, 0 };
	CURLcode rc;
	CURL *c = curl_easy_init();

	if (!c)
		return NULL;
	curl_easy_setopt(c, CURLOPT_URL, url);
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 20L);
	curl_easy_setopt(c, CURLOPT_USERAGENT, "blkcat");
	rc = curl_easy_perform(c);
	curl_easy_cleanup(c);
	if (rc != CURLE_OK) {
		free(b.data);
		return NULL;
	}
	return b.data;
}

static const char *skip_ws(const char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
		s++;
	return s;
}

static const char *find_object(const char *s, const char *key)
{
	char pat[128];
	size_t pl;
	const char *p = s;

	snprintf(pat, sizeof pat, "\"%s\"", key);
	pl = strlen(pat);
	while ((p = strstr(p, pat))) {
		const char *q = skip_ws(p + pl);

		if (*q == ':') {
			q = skip_ws(q + 1);
			if (*q == '{')
				return q + 1;
		}
		p += pl;
	}
	return NULL;
}

static bool find_int(const char *s, const char *key, long *out)
{
	char pat[128];
	size_t pl;
	const char *p = s;

	snprintf(pat, sizeof pat, "\"%s\"", key);
	pl = strlen(pat);
	while ((p = strstr(p, pat))) {
		const char *q = skip_ws(p + pl);

		if (*q == ':') {
			char *end;
			long v;

			q = skip_ws(q + 1);
			v = strtol(q, &end, 10);
			if (end != q) {
				*out = v;
				return true;
			}
		}
		p += pl;
	}
	return false;
}

static bool class_field(const char *classes, const char *cls, const char *field,
			long *out)
{
	const char *c = find_object(classes, cls);
	const char *f;

	if (!c)
		return false;
	f = find_object(c, "fields");
	if (!f)
		return false;
	return find_int(f, field, out);
}

int cs2_offsets_load(void)
{
	char *o, *s;
	const char *client, *engine, *root, *classes;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	o = http_get(OFFSETS_URL);
	s = http_get(SCHEMA_URL);
	if (!o || !s) {
		fprintf(stderr, "blkcat: failed to fetch offsets\n");
		goto fail;
	}

	client = find_object(o, "libclient.so");
	engine = find_object(o, "libengine2.so");
	if (!client || !engine) {
		fprintf(stderr, "blkcat: offsets.json layout unexpected\n");
		goto fail;
	}

#define GET_INT(obj, key, dst)						\
	do {								\
		long v;							\
		if (!find_int((obj), (key), &v)) {			\
			fprintf(stderr, "blkcat: missing %s\n", (key));	\
			goto fail;					\
		}							\
		(dst) = (uintptr_t)v;					\
	} while (0)

	GET_INT(client, "dwGameEntitySystem", g_off.dw_game_entity_system);
	GET_INT(client, "dwLocalPlayerController", g_off.dw_local_player_controller);
	GET_INT(client, "dwViewMatrix", g_off.dw_view_matrix);
	GET_INT(engine, "dwWindowWidth", g_off.dw_window_width);
	GET_INT(engine, "dwWindowHeight", g_off.dw_window_height);

	root = find_object(s, "libclient.so");
	classes = root ? find_object(root, "classes") : NULL;
	if (!classes) {
		fprintf(stderr, "blkcat: libclient_so.json layout unexpected\n");
		goto fail;
	}

#define GET_FIELD(cls, field, dst)					\
	do {								\
		long v;							\
		if (!class_field(classes, (cls), (field), &v)) {		\
			fprintf(stderr, "blkcat: missing %s.%s\n",	\
				(cls), (field));			\
			goto fail;					\
		}							\
		(dst) = (uintptr_t)v;					\
	} while (0)

	GET_FIELD("CCSPlayerController", "m_hPlayerPawn", g_off.m_h_player_pawn);
	GET_FIELD("C_BaseEntity", "m_pGameSceneNode", g_off.m_p_game_scene_node);
	GET_FIELD("C_BaseEntity", "m_iHealth", g_off.m_i_health);
	GET_FIELD("C_BaseEntity", "m_lifeState", g_off.m_life_state);
	GET_FIELD("C_BaseEntity", "m_iTeamNum", g_off.m_i_team_num);
	GET_FIELD("CGameSceneNode", "m_vecAbsOrigin", g_off.m_vec_abs_origin);
	GET_FIELD("C_BaseModelEntity", "m_vecViewOffset", g_off.m_vec_view_offset);

#undef GET_INT
#undef GET_FIELD

#define TRY_FIELD(cls, field, dst)					\
do {									\
	long v;								\
	if (class_field(classes, (cls), (field), &v))			\
		(dst) = (uintptr_t)v;					\
	else								\
		fprintf(stderr, "blkcat: warning: %s.%s missing from schema\n", \
			(cls), (field));				\
} while (0)

	TRY_FIELD("CBasePlayerController", "m_iszPlayerName",
		  g_off.m_isz_player_name);
	TRY_FIELD("CSkeletonInstance", "m_modelState", g_off.m_model_state);
	TRY_FIELD("C_BasePlayerPawn", "m_pWeaponServices",
		  g_off.m_p_weapon_services);
	TRY_FIELD("CPlayer_WeaponServices", "m_hActiveWeapon",
		  g_off.m_h_active_weapon);
	TRY_FIELD("C_EconEntity", "m_AttributeManager",
		  g_off.m_attribute_manager);
	TRY_FIELD("C_AttributeContainer", "m_Item", g_off.m_item);
	TRY_FIELD("C_EconItemView", "m_iItemDefinitionIndex",
		  g_off.m_item_definition_index);
	TRY_FIELD("C_CSPlayerPawn", "m_iIDEntIndex", g_off.m_i_id_ent_index);

#undef TRY_FIELD

	fprintf(stderr, "blkcat: offsets loaded (entity_system %#lx, local_ctrl %#lx, view %#lx, h_player_pawn %#lx, health %#lx, team %#lx)\n",
		(unsigned long)g_off.dw_game_entity_system,
		(unsigned long)g_off.dw_local_player_controller,
		(unsigned long)g_off.dw_view_matrix,
		(unsigned long)g_off.m_h_player_pawn,
		(unsigned long)g_off.m_i_health,
		(unsigned long)g_off.m_i_team_num);

	free(o);
	free(s);
	curl_global_cleanup();
	return 0;

fail:
	free(o);
	free(s);
	curl_global_cleanup();
	return -1;
}
