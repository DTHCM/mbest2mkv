#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mbestformat.h"
#include "util.h"

static void replace_lf_by_nul(char *s)
{
	char *iter = s;
	while (*iter) {
		iter = strchr(iter, '\n');
		if (!iter)
			return;
		*iter = '\0';
		iter++;
	}
}

struct mbest_format_t *mbest_format_init(const char *filename)
{
	// calloc ensures memory set to zero.
	struct mbest_format_t *ret = xcalloc(1, sizeof(struct mbest_format_t));
	size_t len;
	if (!readfile(filename, &ret->_buf, &len, true))
		goto err;

	size_t line = 0;
	char *fps, *frames, *width, *height;
	replace_lf_by_nul(ret->_buf);

	char *iter = ret->_buf;

	while (iter && iter < ret->_buf + len && line < 4) {
		line++;
		if (line == 1)
			fps = iter;
		else if (line == 2)
			frames = iter;
		else if (line == 3)
			width = iter;
		else if (line == 4)
			height = iter;

		iter = strchr(iter, '\0');
		if (iter)
			iter++;
	}

	if (line < 4)
		goto err;

        ret->fps = atof(fps);
	ret->frames = atoi(frames);
	ret->width = atoi(width);
	ret->height = atoi(height);

	if (!ret->fps) {
		fprintf(stderr, "%s: Invalid fps %s, returning NULL\n", __FUNCTION__, fps);
		goto err;
	}

	if (!ret->frames) {
		fprintf(stderr, "%s: Invalid frames %s, returning NULL\n", __FUNCTION__, frames);
		goto err;
	}

	if (!ret->width) {
		fprintf(stderr, "%s: Invalid width %s, returning NULL\n", __FUNCTION__, width);
		goto err;
	}

	if (!ret->height) {
		fprintf(stderr, "%s Invalid height %s, returning NULL\n", __FUNCTION__, height);
		goto err;
	}

	ret->frame_info = xmalloc(sizeof(char*)*ret->frames);

	size_t frame_cnt = 0;
	while (iter && iter < ret->_buf + len && frame_cnt < ret->frames) {
		ret->frame_info[frame_cnt++] = iter;

		iter = strchr(iter, '\0');
		if (iter)
			iter++;
	}

	return ret;

err:
	fflush(stderr);
	if (!ret) return NULL;

	if (ret->_buf)
		free(ret->_buf);
	if (ret->frame_info)
		free(ret->frame_info);
	free(ret);

	return NULL;
}

void mbest_format_free(struct mbest_format_t *fmt)
{
	free(fmt->_buf);
	free(fmt->frame_info);
	free(fmt);
}

bool mbest_read_frame(const struct mbest_format_t *fmt,
		unsigned int frame, char **buf, size_t *len)
{
	// Shame on you!
	if (frame > fmt->frames) {
		*buf = NULL;
		*len = 0;
		return false;
	}

	return readfile(mbest_frame_info_filename(fmt->frame_info[frame]),
			buf, len, false);
}

static inline const char frame_info_desc_char(char *info)
{
	return *info;
}

bool mbest_is_keyframe(const struct mbest_format_t *fmt, unsigned int frame)
{
	if (frame > fmt->frames)
		return false;

	return frame_info_desc_char(fmt->frame_info[frame]) == '*';
}

bool mbest_is_discardable(const struct mbest_format_t *fmt, unsigned int frame)
{
	if (frame > fmt->frames)
		return false;

	return frame_info_desc_char(fmt->frame_info[frame]) == '-';
}

const char *mbest_frame_info_filename(char *info)
{
	return info + 1;
}

