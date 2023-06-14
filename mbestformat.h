#ifndef _MBESTFORMAT_H
#define _MBESTFORMAT_H

#include <stddef.h>
#include <stdbool.h>

/* frame starts from 0. */

struct mbest_format_t
{
	double fps;
	unsigned int frames;
	unsigned int width;
	unsigned int height;

	char *_buf;
	char **frame_info;
};

/**
 * Create an instance of mbest_format_t.
 *
 * @param filename file name of mbest-formatted file.
 */
struct mbest_format_t *mbest_format_init(const char *filename);
void mbest_format_free(struct mbest_format_t* fmt);

bool mbest_read_frame(const struct mbest_format_t *fmt,
		unsigned int frame, char **buf, size_t *len);

bool mbest_is_keyframe(const struct mbest_format_t *fmt, unsigned int frame);
bool mbest_is_discardable(const struct mbest_format_t *fmt, unsigned int frame);

const char *mbest_frame_info_filename(char *info);

#endif

