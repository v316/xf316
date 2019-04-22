/*
 * 2013.09.05
 * yangy@allwinnertech.com
 *
 * Copyright (C) 2013 Allwinner Technology.
 */

#ifndef _GAL_sunxi_h
#define _GAL_sunxi_h

#include "../sysvideo.h"
#include "g2d.h"
#include "../videomem-bucket.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	GAL_VideoDevice *this

/* Private display data */

struct GAL_PrivateVideoData { /* for sunxi only */
    int fd_g2d;
    int fd_fb;
    int fd_disp;
    unsigned int smem_start; /* physics address */
    unsigned int smem_len;

    unsigned char *pixels;
    int width;
    int height;
    int pitch;

    gal_vmbucket_t vmbucket;
};

struct private_hwdata { /* for GAL_SURFACE */
    gal_vmblock_t *vmblock;
    unsigned int addr_phy;
    int fd_g2d;
    int fd_disp;
};

typedef unsigned int u32;

#endif /* _GAL_sunxi_h */
