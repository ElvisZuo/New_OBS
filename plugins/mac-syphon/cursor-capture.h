#pragma once

#include <obs-module.h>

struct mac_cursor {
	gs_texture_t *tex;
	uint8_t *image;
	long x;
	long y;
	long cx;
	long cy;
	long hotspot_x;
	long hotspot_y;
	bool visible;
	bool bad_cursor;
};

extern void mac_cursor_init(struct mac_cursor *cursor);
extern void mac_cursor_free(struct mac_cursor *cursor);
extern void mac_cursor_update(struct mac_cursor *cursor);
extern void mac_cursor_render(struct mac_cursor *cursor,
		long rel_x, long rel_y, long width, long height);
