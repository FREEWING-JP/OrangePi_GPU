/*
 * Copyright © 2013 ARM Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "../drmmode_driver.h"
#include "sunxi_drm.h"
#include <stddef.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <sys/ioctl.h>

#include "armsoc_driver.h"
#include "../armsoc_dumb.h"

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
 struct armsoc_device {
	int fd;
	int (*create_custom_gem)(int fd, struct armsoc_create_gem *create_gem);
	Bool alpha_supported;
};
struct armsoc_bo {
	struct armsoc_device *dev;
	uint32_t handle;
	uint32_t size;
	void *map_addr;
	uint32_t fb_id;
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	uint8_t bpp;
	uint32_t pitch;
	int refcnt;
	int dmabuf;
	uint32_t original_size;
	uint32_t name;
};
/* width */
#define CURSORW   (64)
/* height */
#define CURSORH   (64)
/* Padding added down each side of cursor image */
#define CURSORPAD (0)

/* Optional function only for HWCURSOR_API_PLANE interface */

void sunxi_sync_gem(struct armsoc_bo *src_bo)
{
    int ret;
    struct sunxi_sync_gem_cmd sync_cmd;
    sync_cmd.gem_handle = src_bo->handle;
    ret = ioctl(src_bo->dev->fd, DRM_IOCTL_SUNXI_SYNC_GEM, &sync_cmd);
	if(ret) {
        xf86Msg(X_ERROR,"rotate commit err.\n");
	} 
}
static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_mode_create_dumb create_sunxi;
	int ret;

	/* make pitch a multiple of 64 bytes for best performance */
	memset(&create_sunxi, 0, sizeof(create_sunxi));
	create_sunxi.width = create_gem->width;
    create_sunxi.height = create_gem->height;
    create_sunxi.bpp = create_gem->bpp;
	create_sunxi.flags = SUNXI_BO_CONTIG|SUNXI_BO_CACHABLE;
    xf86Msg(X_INFO, "######flags:%x\n",create_sunxi.flags);

	assert((create_gem->buf_type == ARMSOC_BO_SCANOUT) ||
			(create_gem->buf_type == ARMSOC_BO_NON_SCANOUT));
//	if(create_gem->buf_type == ARMSOC_BO_SCANOUT)
	 create_sunxi.flags = SUNXI_BO_CONTIG|SUNXI_BO_WC;
	/* Contiguous allocations are not supported in some sunxi drm versions.
	 * When they are supported all allocations are effectively contiguous
	 * anyway, so for simplicity we always request non contiguous buffers.
	 */

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_sunxi);
	if (ret)
		return ret;

	/* Convert custom create_sunxi to generic create_gem */
	create_gem->handle = create_sunxi.handle;
	create_gem->pitch = create_sunxi.pitch;
	create_gem->size = create_sunxi.size;

	return 0;
}

struct drmmode_interface sunxi_interface = {
    	"sunxi",
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_NONE   /* cursor_api */,
	NULL                /* init_plane_for_cursor */,
	0                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};
