#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

struct config {
	bool box;
	bool full_box;
	int esp_thickness;
	bool esp_outline;
	int esp_outline_thickness;
	bool esp_skeleton;
	bool name;
	bool health_bar;
	bool health_text;
	bool weapon;
	bool distance;
	bool head_circle;

	bool triggerbot;
	int trigger_hotkey;
	int trigger_delay;
	int trigger_jitter;
	bool trigger_jitter_pct;
	int trigger_hold;
	int trigger_hold_jitter;
	int trigger_next;

	unsigned long box_color;
	unsigned long outline_color;
	unsigned long skeleton_color;
	unsigned long head_circle_color;
	unsigned long name_color;
	unsigned long health_color;
	unsigned long health_text_color;
	unsigned long weapon_color;
	unsigned long distance_color;
};

extern struct config g_cfg;

void config_load(const char *path);
void config_reload_if_changed(void);
void config_save(void);

#endif
