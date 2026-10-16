#ifndef CS2_H
#define CS2_H

#include <stdbool.h>
#include <stdint.h>

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;

#define CS2_ENT_STRIDE      0x70
#define CS2_ENT_CHUNK_BASE  0x10
#define CS2_ENT_CHUNK_SHIFT 9
#define CS2_ENT_CHUNK_MASK  0x1FF
#define CS2_HANDLE_MASK     0x7FFF

#define CS2_MAX_PLAYERS     64
#define CS2_TEAM_T          2
#define CS2_TEAM_CT         3

#define CS2_BONE_ARRAY_OFF  0x80
#define CS2_BONE_STRIDE     0x20
#define CS2_BONE_COUNT      30

struct cs2_offsets {
	uintptr_t dw_game_entity_system;
	uintptr_t dw_local_player_controller;
	uintptr_t dw_view_matrix;
	uintptr_t dw_window_width;
	uintptr_t dw_window_height;
	uintptr_t m_h_player_pawn;
	uintptr_t m_p_game_scene_node;
	uintptr_t m_i_health;
	uintptr_t m_life_state;
	uintptr_t m_i_team_num;
	uintptr_t m_vec_abs_origin;
	uintptr_t m_vec_view_offset;
	uintptr_t m_isz_player_name;
	uintptr_t m_model_state;
	uintptr_t m_p_weapon_services;
	uintptr_t m_h_active_weapon;
	uintptr_t m_attribute_manager;
	uintptr_t m_item;
	uintptr_t m_item_definition_index;
	uintptr_t m_i_id_ent_index;
};

extern struct cs2_offsets g_off;

#endif
