#include <obs-module.h>
#include "cursor-capture.h"

#import <CoreGraphics/CGEvent.h>
#import <Cocoa/Cocoa.h>

static void update_cursor(struct mac_cursor *info);

void mac_cursor_init(struct mac_cursor *cursor)
{
	memset(cursor, 0, sizeof(*cursor));
}

void mac_cursor_free(struct mac_cursor *cursor)
{
	if (cursor->image)
		bfree(cursor->image);
	if (cursor->tex) {
		obs_enter_graphics();
		gs_texture_destroy(cursor->tex);
		obs_leave_graphics();
	}
}

void mac_cursor_update(struct mac_cursor *cursor)
{
	@autoreleasepool {
		update_cursor(cursor);
	}
}

void mac_cursor_render(struct mac_cursor *cursor, long rel_x, long rel_y,
		long width, long height)
{
	if (!cursor->tex || !cursor->visible)
		return;

	long x = cursor->x - rel_x;
	long y = cursor->y - rel_y;
	long x_draw = x - cursor->hotspot_x;
	long y_draw = y - cursor->hotspot_y;

	if (x < 0 || x > width || y < 0 || y > height)
		return;

	obs_source_draw(cursor->tex, x_draw, y_draw, 0, 0, false);
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static void update_cursor(struct mac_cursor *info)
{
	info->visible = CGCursorIsVisible();
	if (!info->visible)
		return;

	NSCursor *cursor = [NSCursor currentSystemCursor];
	NSImage *image = [cursor image];
	NSData *tiff = [image TIFFRepresentation];
	NSBitmapImageRep *bmp = [NSBitmapImageRep imageRepWithData:tiff];

	/* XXX: are cursor bitmaps other than 32bit ever used? */
	long colors = bmp.samplesPerPixel;
	long bpp = bmp.bitsPerPixel;
	if (colors != 4 || bpp != 32) {
		if (!info->bad_cursor)
			blog(LOG_DEBUG, "weird cursor: %ld colors, %ld bpp",
					colors, bpp);
		info->visible = false;
		info->bad_cursor = true;
		return;
	}

	info->bad_cursor = false;

	long row_bytes = bmp.bytesPerRow;
	long cx = [bmp pixelsWide];
	long cy = [bmp pixelsHigh];
	NSPoint hotspot = [cursor hotSpot];
	bool update = false;

	long size = row_bytes * cy;

	CGEventRef event = CGEventCreate(NULL);
	CGPoint pos = CGEventGetLocation(event);
	CFRelease(event);

	info->x = (long)pos.x;
	info->y = (long)pos.y;
	info->hotspot_x = (long)hotspot.x;
	info->hotspot_y = (long)hotspot.y;

	const unsigned char *data = bmp.bitmapData;
	if (!info->image) {
		update = true;
	} else {
		update = cx != info->cx || cy != info->cy ||
			memcmp(info->image, data, size) != 0;
	}

	if (!update || !size)
		return;

	if (info->image)
		bfree(info->image);
	info->image = bmalloc(size);
	memcpy(info->image, data, size);

	obs_enter_graphics();

	if (info->tex)
		gs_texture_destroy(info->tex);
	info->tex = gs_texture_create(cx, cy, GS_RGBA, 1, &data, 0);

	obs_leave_graphics();

	info->cx = cx;
	info->cy = cy;
}
