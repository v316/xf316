/*
 * 2013.09.05
 * yangy@allwinnertech.com
 *
 * Copyright (C) 2013 Allwinner Technology.
 */

#define DEBUG
#ifndef DEBUG
#include <stdio.h>
#define printf(...) do{}while(0)
#define sunxi_dbg(...)
#endif
#define dbg() printf("%s %d\n", __FUNCTION__, __LINE__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#include "common.h"
#include "minigui.h"
#include "newgal.h"
#include "sysvideo.h"
#include "pixels_c.h"

#ifdef _MGGAL_SUNXI

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>


#ifdef _MGRM_PROCESSES
#   include "client.h"
#endif
#include "sunxi.h"
#include "drv_display.h"
#define SUNXI_DRIVER_NAME "sunxi"

#ifdef _MGRM_PROCESSES
#   define ISSERVER (mgIsServer)
#else
#   define ISSERVER (1)
#endif

static inline void sunxi_dbg(const char *fmt, ...)
{
	va_list argp;
	fprintf(stderr, "[sunxi]: ");
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
} 




extern void (*__mg_ial_change_mouse_xy_hook)(int* x, int* y);
static int fb_layer = 0;
static unsigned int *addr_vir;
static unsigned int *addr_phy;
int colorkey_color = 0x000000;
int disp_fd = -1;
extern int hw_mem_fd;
/* Initialization/Query functions */
static int
	SUNXI_VideoInit(_THIS, GAL_PixelFormat *vformat);
static GAL_Rect **
	SUNXI_ListModes(_THIS, GAL_PixelFormat *format, Uint32 flags);
static GAL_Surface *
	SUNXI_SetVideoMode(_THIS, GAL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int
	SUNXI_SetColors(_THIS, int firstcolor, int ncolors, GAL_Color *colors);
static void
	SUNXI_VideoQuit(_THIS);

/* Hardware surface functions */
#ifdef _MGRM_PROCESSES
static void
	SUNXI_RequestHWSurface(_THIS, const REQ_HWSURFACE * request, REP_HWSURFACE * reply) (_THIS, const REQ_HWSURFACE* request, REP_HWSURFACE* reply);
#endif
static int
	SUNXI_AllocHWSurface(_THIS, GAL_Surface *surface);
static void
	SUNXI_FreeHWSurface(_THIS, GAL_Surface *surface);
static int
	SUNXI_CheckHWBlit(_THIS, GAL_Surface * src, GAL_Surface * dst);
static int
	SUNXI_HWAccelBlit(GAL_Surface * src, GAL_Rect * srcrect, GAL_Surface * dst, GAL_Rect * dstrect);

static int
	SUNXI_BitBlt_Overlap(RECT *ol, GAL_Surface *src, GAL_Rect *srcrect, GAL_Surface *dst, GAL_Rect *dstrect);
static int
	SUNXI_BitBlt_Normal(GAL_Surface *src, GAL_Rect *srcrect, GAL_Surface *dst, GAL_Rect *dstrect);
static int
	SUNXI_FillHWRect(_THIS, GAL_Surface *dst, GAL_Rect *rect, Uint32 color);
static unsigned int 
	get_colormode(GAL_Surface *surface);


/* SUNXI driver bootstrap functions */

static int SUNXI_Available(void)
{
    return(1);
}

static void SUNXI_DeleteDevice(GAL_VideoDevice *device)
{
    if (ISSERVER) {
        gal_vmbucket_destroy(&device->hidden->vmbucket);
    }
    free(device->hidden);
    free(device);
}

static int SUNXI_SetHWAlpha(_THIS, GAL_Surface *surface, Uint8 value)
{
	surface->format->alpha = value;
    surface->flags |= GAL_SRCALPHA;

	return 0;
}
static int SUNXI_FillHWRect(_THIS, GAL_Surface *dst, GAL_Rect *rect, Uint32 color)
{
	g2d_fillrect fillrect;
	memset(&fillrect, 0, sizeof(g2d_fillrect));
	if (dst == NULL || rect == NULL) {
        fprintf(stderr, "SUNXI_FillHWRect >> dst or dstrect NULL\n");
        return -1;
    }

    if (rect->w <= 0 || rect->h <= 0) {
        fprintf(stderr, "SUNXI_FillHWRect >> dstrect invaildate\n");
        return -1;
	}

    if (dst->hwdata == NULL) {
        fprintf(stderr, "SUNXI_FillHWRect >> dst hwdata NULL\n");
        return -1;
    }
	
	if ((dst->flags & GAL_SRCALPHA)) {
	    	fillrect.flag |= G2D_BLT_PLANE_ALPHA;
	    	fillrect.alpha = dst->format->alpha;
	} 
	else if (dst->flags & GAL_SRCPIXELALPHA) {
		fillrect.flag |= G2D_BLT_PIXEL_ALPHA;
		fillrect.alpha = dst->format->alpha;
	} 
	fillrect.color				= color;
	/* 设置目标image和目标rect */
	fillrect.dst_image.addr[0]	= dst->hwdata->addr_phy;
	fillrect.dst_image.w		= dst->w;
	fillrect.dst_image.h		= dst->h;
	fillrect.dst_image.format	= get_colormode(dst);
	fillrect.dst_rect.x			= rect->x;
	fillrect.dst_rect.y			= rect->y;
	fillrect.dst_rect.w			= rect->w;
	fillrect.dst_rect.h			= rect->h;
	if(ioctl(dst->hwdata->fd_g2d, G2D_CMD_FILLRECT, &fillrect)<0)
	{
		return -1;
		printf("G2D_CMD_FILLRECT failed!\n");
	}
	
	return 0;
}


static GAL_VideoDevice *SUNXI_CreateDevice(int devindex)
{
    GAL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (GAL_VideoDevice *)malloc(sizeof(GAL_VideoDevice));
    if ( device ) {
        memset(device, 0, (sizeof *device));
        device->hidden = (struct GAL_PrivateVideoData *)
                malloc((sizeof *device->hidden));
    }
    if ( (device == NULL) || (device->hidden == NULL) ) {
        GAL_OutOfMemory();
        if ( device ) {
            free(device);
        }
        return(0);
    }
    memset(device->hidden, 0, (sizeof *device->hidden));

    /* Set the function pointers */
    device->VideoInit		= SUNXI_VideoInit;
    device->ListModes		= SUNXI_ListModes;
    device->SetVideoMode	= SUNXI_SetVideoMode;
    device->SetColors		= SUNXI_SetColors;
    device->VideoQuit		= SUNXI_VideoQuit;
#ifdef _MGRM_PROCESSES
    device->RequestHWSurface = SUNXI_RequestHWSurface;
#endif
    device->AllocHWSurface	= SUNXI_AllocHWSurface;
    device->CheckHWBlit		= SUNXI_CheckHWBlit;
#ifdef SUNXI_G2D
    device->FillHWRect		= SUNXI_FillHWRect;
#endif
    device->SetHWColorKey	= NULL;
    device->SetHWAlpha		= NULL;
    device->FreeHWSurface	= SUNXI_FreeHWSurface;

    device->free			= SUNXI_DeleteDevice;

    return device;
}

VideoBootStrap SUNXI_bootstrap = {
    SUNXI_DRIVER_NAME, "SUNXI video driver",
    SUNXI_Available, SUNXI_CreateDevice
};

static void GetDefaultLayer(int fd_fb)
{
	ioctl(fd_fb,FBIOGET_LAYER_HDL_0,&fb_layer);
	sunxi_dbg("-------fb0_layer:%d\n", fb_layer);
}
static int LayerInit(int fd_fb)
{
	GetDefaultLayer(fd_fb);

	return 0;
}

int SetUITop(void)
{
	unsigned int args[4]={0};
	
	args[0] = 0;
	args[1] = fb_layer;
	ioctl(disp_fd,DISP_CMD_LAYER_TOP,(unsigned int)args);
	
	return 0;
}
static int SUNXI_VideoInit(_THIS, GAL_PixelFormat *vformat)
{
    struct GAL_PrivateVideoData *hidden;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    int fd_fb	= -1;
    int fd_g2d	= -1;
    unsigned char *pixels = MAP_FAILED;

    fd_fb = open("/dev/graphics/fb0", O_RDWR);
    if (fd_fb < 0) {
        fprintf(stderr,"open() fb0\n");
        goto err;
    }
#ifdef SUNXI_G2D
    fd_g2d = open("/dev/g2d", O_RDWR);
    if (fd_g2d < 0) {
        fprintf(stderr,"open() g2d\n");
        goto err;
    }
#endif
    disp_fd = open("/dev/disp", O_RDWR);
    if (disp_fd < 0) {
        fprintf(stderr,"open() disp\n");
        goto err;
    }
	hwmem_init();

    if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &finfo) < 0) {
        fprintf(stderr,"ioctl(FBIOGET_FSCREENINFO)");
        goto err;
    }
    assert(finfo.smem_start && finfo.smem_len);

    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        fprintf(stderr,"ioctl(FBIOGET_VSCREENINFO)");
        goto err;
    }

	/* get fb vir_mem */
    pixels = (unsigned char *) mmap(NULL, finfo.smem_len,
            PROT_READ|PROT_WRITE, MAP_SHARED, fd_fb, 0);
    if (pixels == MAP_FAILED) {
        fprintf(stderr,"mmap()");
        goto err;
    }

    /* Determine the screen depth (use default 8-bit depth) */
    /* we change this during the GAL_SetVideoMode implementation... */
    vformat->BitsPerPixel = 8;
    vformat->BytesPerPixel = 1;

    this->hidden		= (struct GAL_PrivateVideoData *)calloc(1, sizeof(struct GAL_PrivateVideoData));
    hidden				= this->hidden;
    hidden->fd_fb		= fd_fb;
    hidden->fd_g2d		= fd_g2d;
    hidden->fd_disp		= disp_fd;
    hidden->smem_start	= finfo.smem_start;
    hidden->smem_len	= finfo.smem_len;
    hidden->pitch		= finfo.line_length;
    hidden->pixels		= pixels;
    hidden->width		= vinfo.width;
    hidden->height		= vinfo.height;
    if (ISSERVER) {
#if defined(SUNXI_MEM) || defined(ION_MEM)
	addr_vir = hwmem_virmalloc(HWMEM_SIZE);
	sunxi_dbg("vir-=-------------:%x\n", addr_vir);
	addr_phy = hwmem_vir2phy(addr_vir);
	sunxi_dbg("phy-=-------------:%x\n", addr_phy);
	gal_vmbucket_init(&hidden->vmbucket, addr_vir, HWMEM_SIZE);
#else
    memset (hidden->pixels, 0, hidden->smem_len);
    gal_vmbucket_init(&hidden->vmbucket, hidden->pixels, hidden->smem_len);
#endif
    }

    {
        GAL_VideoInfo *video_info = &this->info;
        video_info->blit_fill	= 1;
        video_info->blit_hw		= 1;
        video_info->blit_hw_CC	= 1;
        video_info->blit_hw_A	= 1;
    }
	printf("before layer init: %s %d\n", __FUNCTION__, __LINE__);
	printf("before layer init:\n");
	LayerInit(fd_fb);
    return(0);

err:
    assert(0);
    if (pixels != MAP_FAILED) {
        munmap(pixels, finfo.smem_len);
    }
    if (fd_fb >= 0) {
        close(fd_fb);
    }
    if (fd_g2d >= 0) {
        close(fd_g2d);
    }
	if (disp_fd >= 0) {
		close(disp_fd);
	}
	hwmem_exit();
    return -1;
}

static GAL_Rect **SUNXI_ListModes(_THIS, GAL_PixelFormat *format, Uint32 flags)
{
	/* any mode supported */
 	return (GAL_Rect**) -1;
}

static GAL_Surface *SUNXI_SetVideoMode(_THIS, GAL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    struct GAL_PrivateVideoData *hidden = this->hidden;

    /* Allocate the new pixel format for the screen */
    if (!GAL_ReallocFormat (current, bpp, 0xFF0000, 0xFF00, 0xFF, 0xFF000000)) {
        fprintf (stderr, "NEWGAL>SUNXI: "
                "Couldn't allocate new pixel format for requested mode\n");
        return(NULL);
    }

    /* Set up the new mode framebuffer */
    current->w		= width;
    current->h		= height;
    current->pitch	= hidden->pitch;

    if (ISSERVER) {
        SUNXI_AllocHWSurface(this, current);
    } else {
        current->hwdata = (struct private_hwdata *)calloc(1, sizeof(struct private_hwdata));
        current->hwdata->vmblock = NULL;
        current->hwdata->addr_phy = hidden->smem_start + 0;
        current->hwdata->fd_g2d = hidden->fd_g2d;
		current->hwdata->fd_disp = hidden->fd_disp;
        current->pixels = hidden->pixels + 0;
        current->pitch = this->hidden->pitch;
        current->flags |= GAL_HWSURFACE;
    }
    return current;
}

static int
SUNXI_CheckHWBlit(_THIS, GAL_Surface * src, GAL_Surface * dst)
{
    src->flags &= ~GAL_HWACCEL;
    src->map->hw_blit = NULL;

    // only supported the hw surface accelerated.
    if (!(src->flags & GAL_HWSURFACE) || !(dst->flags & GAL_HWSURFACE))
    {
        printf("src(%s) dst(%s)\n", 
                (src->flags & GAL_HWSURFACE) ? "HW" : "SW",
                (dst->flags & GAL_HWSURFACE) ? "HW" : "SW");
        return -1;
    }

    src->flags |= GAL_HWACCEL;
#ifdef SUNXI_G2D
    src->map->hw_blit = SUNXI_HWAccelBlit;
#endif
    return 0;
}

static unsigned int get_colormode(GAL_Surface *surface)
{
    GAL_PixelFormat *format = surface->format;
    switch (format->BytesPerPixel) {
        case 2:
            if (1
                    && format->Amask == 0x0000
                    && format->Rmask == 0xF800
                    && format->Gmask == 0x07E0
                    && format->Bmask == 0x001F) {
                return G2D_FMT_RGB565;
            }else if (1
                    && format->Rmask == 0x7C00
                    && format->Gmask == 0x03E0
                    && format->Bmask == 0x001F
                    ) {
                if ((surface->flags & GAL_SRCPIXELALPHA) && format->Amask == 0x8000){
                    return G2D_FMT_ARGB1555;
                }else{
                    break;
                }
            }else if (1
                    && format->Rmask == 0xF800
                    && format->Gmask == 0x07C0
                    && format->Bmask == 0x003E){
                if ((surface->flags & GAL_SRCPIXELALPHA) && format->Amask == 0x0001) {
                    return G2D_FMT_RGBA5551;
                }else{
                    break;
                }
            }else{
                break;
            }
		case 3:
            if (1
                    && format->Amask == 0x000000
                    && format->Rmask == 0xFF0000
                    && format->Gmask == 0x00FF00
                    && format->Bmask == 0x0000FF) {
                return G2D_FMT_XRGB8888;
            }else if (1
            		&& format->Amask == 0x00000F
                    && format->Rmask == 0x00F000
                    && format->Gmask == 0x000F00
                    && format->Bmask == 0x0000F0) {
            	return G2D_FMT_RGBA4444;
            }
            else{
                break;
            }
        case 4:
            if (1
                    && format->Rmask == 0x00FF0000
                    && format->Gmask == 0x0000FF00
                    && format->Bmask == 0x000000FF){
                if ((surface->flags & GAL_SRCPIXELALPHA) && format->Amask == 0xFF000000) {
                    return G2D_FMT_ARGB_AYUV8888;
                }else{
                    return G2D_FMT_XRGB8888;
                }
            }else if (1
                    && format->Rmask == 0xFF000000
                    && format->Gmask == 0x00FF0000
                    && format->Bmask == 0x0000FF00){
                if ((surface->flags & GAL_SRCPIXELALPHA) && format->Amask == 0x000000FF) {
                    return G2D_FMT_RGBA_YUVA8888;
                }else{
                    return G2D_FMT_RGBX8888;
                }
            }else if (1
                    && format->Rmask == 0x00F80000
                    && format->Gmask == 0x00007E00
                    && format->Bmask == 0x0000001F){
                    return G2D_FMT_RGB565;
            }else{
                break;
            }
        default:
            break;
    }
    fprintf(stderr, "Format(Bpp:%d A:%08x R:%08x G:%08x B:%08x) not supported\n",
            format->BytesPerPixel, format->Amask, format->Rmask, format->Gmask, format->Bmask);
    assert(0);
    return (unsigned int)-1;
}

void print_args(g2d_blt blt)
{
	printf("src->hwdata->addr_phy:%x\n", blt.src_image.addr[0]);
	printf("src->h:%d\n", blt.src_image.h);
	printf("src->w:%d\n", blt.src_image.w );
	printf("srcrect->w:%d\n", blt.src_rect.w);
	printf("srcrect->h:%d\n", blt.src_rect.h);
	printf("srcrect->x:%d\n", blt.src_rect.x);
	printf("srcrect->y:%d\n", blt.src_rect.y);
	printf("dst->hwdata->addr_phy:%x\n", blt.dst_image.addr[0]);
	printf("dst->h:%d\n", blt.dst_image.h);
	printf("dst->w:%d\n", blt.dst_image.w );
	printf("dstrect->x:%d\n", blt.dst_x);
	printf("dstrect->y:%d\n", blt.dst_y);
}

void print_args2(g2d_stretchblt blt)
{
	printf("src->hwdata->addr_phy:%x\n", blt.src_image.addr[0]);
	printf("src->h:%d\n", blt.src_image.h);
	printf("src->w:%d\n", blt.src_image.w );
	printf("srcrect->w:%d\n", blt.src_rect.w);
	printf("srcrect->h:%d\n", blt.src_rect.h);
	printf("srcrect->x:%d\n", blt.src_rect.x);
	printf("srcrect->y:%d\n", blt.src_rect.y);
	printf("dst->hwdata->addr_phy:%x\n", blt.dst_image.addr[0]);
	printf("dst->h:%d\n", blt.dst_image.h);
	printf("dst->w:%d\n", blt.dst_image.w );
	printf("dst_rect->w:%d\n", blt.dst_rect.w);
	printf("dst_rect->h:%d\n", blt.dst_rect.h);
	printf("dst_rect->x:%d\n", blt.dst_rect.x);
	printf("dst_rect->y:%d\n", blt.dst_rect.y);
}

static int IntersectRectOverLap(RECT* overlap, const RECT* src, const RECT* dst)
{

    overlap->left = (src->left > dst->left) ? src->left : dst->left;
    overlap->top  = (src->top > dst->top) ? src->top : dst->top;
    overlap->right = (src->right < dst->right) ? src->right : dst->right;
    overlap->bottom = (src->bottom < dst->bottom) 
                   ? src->bottom : dst->bottom;

    if (overlap->left >= overlap->right || overlap->top >= overlap->bottom)
        return 0;
	
	if (((overlap->left == dst->left) && (overlap->bottom != dst->bottom)) ||
		((overlap->left == src->left) && (overlap->top != src->top)))
    	return 1;
	return 0;
}

/*
src
------------------------------
|      |                      |
-------|----------------------|
|/////////////////////////////|
|//////////1//////////////////|  dst
|-----------------------------------------------------
|//////////////////////|//////|                       |
------------------------------------------------------|
                       |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\|
                       |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\|
                       |\\\\\\\\\\\\\\1\\\\\\\\\\\\\\\|
                       |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\|
                       -------------------------------
 We need to BitBlt 2 parts from src to dst.
*/
static int
SUNXI_BitBlt_Overlap(RECT *ol, GAL_Surface *src, GAL_Rect *srcrect, GAL_Surface *dst, GAL_Rect *dstrect)
{

	g2d_blt blt;
	int ret = 0;
	int remain_y, cp_h, cp_w;
	g2d_data_fmt src_fmt =  get_colormode(src);
	g2d_data_fmt dst_fmt =  get_colormode(dst);
	int off_y;
	cp_h	= dstrect->h - (ol->bottom-ol->top);
	cp_w	= srcrect->w;
	off_y	= srcrect->h - cp_h;
	remain_y= srcrect->h;
	for (;remain_y>0 && (cp_h>0); off_y-=cp_h) {
		memset(&blt, 0, sizeof(g2d_blt));
	    blt.src_image.addr[0]	= src->hwdata->addr_phy;
	    blt.src_image.h			= src->h;
		blt.src_image.w			= src->w;
		blt.dst_image.addr[0]	= dst->hwdata->addr_phy;
		blt.dst_image.h			= dst->h;
		blt.dst_image.w			= dst->w;
		blt.src_image.format	= src_fmt;
		blt.dst_image.format	= dst_fmt;
		
		blt.src_rect.w			= cp_w;
		blt.src_rect.h			= cp_h;
		blt.src_rect.x			= srcrect->x;
		blt.src_rect.y			= srcrect->y +off_y;
		blt.dst_x				= dstrect->x;
		blt.dst_y				= dstrect->y +off_y;
		if ((src->flags & GAL_SRCALPHA)) {
	    	blt.flag |= G2D_BLT_PLANE_ALPHA;
	    	blt.alpha = src->format->alpha;
		} 
		else if (src->flags & GAL_SRCPIXELALPHA) {
			blt.flag |= G2D_BLT_PIXEL_ALPHA;
			blt.alpha = src->format->alpha;
		} 
		if (src->flags & GAL_SRCCOLORKEY) {
			blt.flag |= G2D_BLT_SRC_COLORKEY;
			blt.color = src->format->colorkey;
		}
#ifdef SUNXI_STRETCHBLT
		if ((src->flags & GAL_SUNXI_STRETCH)) {
			g2d_stretchblt blt2;
			memset(&blt2, 0, sizeof(g2d_stretchblt));
			blt2.src_image.addr[0]		= blt.src_image.addr[0];
		    blt2.src_image.h			= blt.src_image.h;
			blt2.src_image.w			= blt.src_image.w;		
			blt2.src_rect.w		    	= blt.src_rect.w;		
			blt2.src_rect.h		    	= blt.src_rect.h;		
			blt2.src_rect.x		    	= blt.src_rect.x;		
			blt2.src_rect.y		    	= blt.src_rect.y;		
			blt2.src_image.format   	= blt.src_image.format;
			blt2.dst_image.addr[0]  	= blt.dst_image.addr[0];
			blt2.dst_image.h			= blt.dst_image.h;
			blt2.dst_image.w			= blt.dst_image.w;		
			blt2.dst_image.format   	= blt.dst_image.format;
			blt2.dst_rect.x				= blt.dst_x;
			blt2.dst_rect.y				= blt.dst_y;
			blt2.dst_rect.w				= dstrect->w;
			blt2.dst_rect.h				= dstrect->h;
			if ((src->flags & GAL_SRCALPHA)) {
	    	blt2.flag |= G2D_BLT_PLANE_ALPHA;
	    	blt2.alpha = src->format->alpha;
			} 
			else if (src->flags & GAL_SRCPIXELALPHA) {
				blt.flag |= G2D_BLT_PIXEL_ALPHA;
				blt2.alpha = src->format->alpha;
			} 
			if (src->flags & GAL_SRCCOLORKEY) {
				blt2.flag |= G2D_BLT_SRC_COLORKEY;
				blt2.color = src->format->colorkey;
			}
			if (ioctl(src->hwdata->fd_g2d, G2D_CMD_STRETCHBLT, &blt2) < 0) {
				fprintf(stderr, "[%d][%s][%s]G2D_CMD_BITBLT failure!\n",__LINE__, __FILE__,__FUNCTION__);
				print_args2(blt2);
				return -1;
			}
		}
		else
#endif
		{
		
			if (ioctl(src->hwdata->fd_g2d, G2D_CMD_BITBLT , &blt) < 0) {
				fprintf(stderr, "[%d][%s][%s]G2D_CMD_BITBLT failure!\n",__LINE__, __FILE__,__FUNCTION__);
				return -1;
			}
		}
		remain_y -= cp_h;
		if (remain_y < cp_h) {
			cp_h = remain_y;
		}
	}
	return ret;
}

static int
SUNXI_BitBlt_Normal(GAL_Surface *src, GAL_Rect *srcrect, GAL_Surface *dst, GAL_Rect *dstrect)
{
	g2d_blt blt;
	memset(&blt, 0, sizeof(g2d_blt));
    blt.src_image.addr[0]	= src->hwdata->addr_phy;
    blt.src_image.h			= src->h;
	blt.src_image.w			= src->w;
	blt.src_rect.w			= srcrect->w;
	blt.src_rect.h			= srcrect->h;
	blt.src_rect.x			= srcrect->x;
	blt.src_rect.y			= srcrect->y;
	blt.src_image.format	= get_colormode(src);
	blt.dst_image.addr[0]	= dst->hwdata->addr_phy;
	blt.dst_image.h			= dst->h;
	blt.dst_image.w			= dst->w;
	blt.dst_image.format	= get_colormode(dst);
	blt.dst_x				= dstrect->x;
	blt.dst_y				= dstrect->y;
	if ((src->flags & GAL_SRCALPHA)) {
	    	blt.flag |= G2D_BLT_PLANE_ALPHA;
	    	blt.alpha = src->format->alpha;
	} 
	else if (src->flags & GAL_SRCPIXELALPHA) {
		blt.flag |= G2D_BLT_PIXEL_ALPHA;
		blt.alpha = src->format->alpha;
	} 
	if (src->flags & GAL_SRCCOLORKEY) {
		blt.flag |= G2D_BLT_SRC_COLORKEY;
		blt.color = src->format->colorkey;
	}
#ifdef SUNXI_STRETCHBLT
	if ((src->flags & GAL_SUNXI_STRETCH)) {
		g2d_stretchblt blt2;
		memset(&blt2, 0, sizeof(g2d_stretchblt));
		blt2.src_image.addr[0]		= blt.src_image.addr[0];
	    blt2.src_image.h			= blt.src_image.h;
		blt2.src_image.w			= blt.src_image.w;		
		blt2.src_rect.w		    	= blt.src_rect.w;		
		blt2.src_rect.h		    	= blt.src_rect.h;		
		blt2.src_rect.x		    	= blt.src_rect.x;		
		blt2.src_rect.y		    	= blt.src_rect.y;		
		blt2.src_image.format   	= blt.src_image.format;
		blt2.dst_image.addr[0]  	= blt.dst_image.addr[0];
		blt2.dst_image.h			= blt.dst_image.h;
		blt2.dst_image.w			= blt.dst_image.w;		
		blt2.dst_image.format   	= blt.dst_image.format;
		blt2.dst_rect.x				= blt.dst_x;
		blt2.dst_rect.y				= blt.dst_y;
		blt2.dst_rect.w				= dstrect->w;
		blt2.dst_rect.h				= dstrect->h;
		if ((src->flags & GAL_SRCALPHA)) {
	    	blt2.flag |= G2D_BLT_PLANE_ALPHA;
	    	blt2.alpha = src->format->alpha;
		} 
		else if (src->flags & GAL_SRCPIXELALPHA) {
			blt2.flag |= G2D_BLT_PIXEL_ALPHA;
			blt2.alpha = src->format->alpha;
		} 
		if (src->flags & GAL_SRCCOLORKEY) {
			blt2.flag |= G2D_BLT_SRC_COLORKEY;
			blt2.color = src->format->colorkey;
		}
		if (ioctl(src->hwdata->fd_g2d, G2D_CMD_STRETCHBLT, &blt2) < 0) {
			fprintf(stderr, "[%d][%s][%s]G2D_CMD_STRETCHBLT failure!\n",__LINE__, __FILE__,__FUNCTION__);
			print_args2(blt2);
			return -1;
		}
	} else 
#endif
	{
	
		if (ioctl(src->hwdata->fd_g2d, G2D_CMD_BITBLT , &blt) < 0) {
			fprintf(stderr, "[%d][%s][%s]G2D_CMD_BITBLT failure!\n",__LINE__, __FILE__,__FUNCTION__);
			return -1;
		}
	}
	return 0;
}

static int
SUNXI_HWAccelBlit(GAL_Surface *src, GAL_Rect *srcrect, GAL_Surface *dst, GAL_Rect *dstrect)
{
	int ret = 0;
	RECT r_ol, r_src, r_dst;

    if (dstrect->w == 0 || dstrect->h == 0) {
        return -1;
    }
    if (src == NULL || srcrect == NULL || dst == NULL || dstrect == NULL)
        return -1;

    if (srcrect->w <= 0 || srcrect->h <= 0 || dstrect->w <= 0 || dstrect->h <= 0) {
        fprintf(stderr, "SUNXI_HWBlit >> srcrect or dstrect invaildate\n");
        return -1;
	}

    if (src->hwdata == NULL || dst->hwdata == NULL) {
        fprintf(stderr, "SUNXI_HWBlit >> src or dst hwdata NULL\n");
        return -1;
		
    }
	if (dstrect->w == 0 || dstrect->h == 0) {
        return -1;
	}
	memset(&r_ol, 0, sizeof(RECT));
	r_src.left	= srcrect->x;
	r_src.right	= srcrect->x + srcrect->w;
	r_src.top	= srcrect->y;
	r_src.bottom= srcrect->y + srcrect->h;
	r_dst.left	= dstrect->x;
	r_dst.right	= dstrect->x + dstrect->w;
	r_dst.top	= dstrect->y;
	r_dst.bottom= dstrect->y + dstrect->h;
	
	if (IntersectRectOverLap(&r_ol, &r_src, &r_dst)) {
		if (SUNXI_BitBlt_Overlap(&r_ol, src, srcrect, dst, dstrect) < 0) {
			printf("failed to bitblt_overlap\n");
			return -1;
		}
	} else {
		if (SUNXI_BitBlt_Normal(src, srcrect, dst, dstrect) < 0) {
			printf("failed to bitblt\n");
			return -1;
		}
	}
    return ret;


}

#ifdef _MGRM_PROCESSES
static void SUNXI_RequestHWSurface (_THIS, const REQ_HWSURFACE* request, REP_HWSURFACE* reply) {
    gal_vmblock_t *block;

    if (request->bucket == NULL) { /* alloc */
        block = gal_vmbucket_alloc(&this->hidden->vmbucket, request->pitch, request->h);
        if (block) {
            reply->offset = block->offset;
            reply->pitch = block->pitch;
            reply->bucket = block;
        }else{
            reply->bucket = NULL;
        }
    }else{ /* free */
        if (request->offset != -1) {
            gal_vmbucket_free(&this->hidden->vmbucket, (gal_vmblock_t *)request->bucket);
        }
    }
}
#endif

/* We don't actually allow hardware surfaces other than the main one */
static int SUNXI_AllocHWSurface(_THIS, GAL_Surface *surface)
{
    int offset, pitch;
    struct GAL_PrivateVideoData *hidden = this->hidden;
    gal_vmblock_t *block;

    if (ISSERVER) {
        /* force 4-byte align */
        if (surface->format->BytesPerPixel != 4) {
            surface->w = (surface->w + 1) & ~1;
            surface->pitch = surface->w * surface->format->BytesPerPixel;
        }

        block = gal_vmbucket_alloc(&hidden->vmbucket, surface->pitch, surface->h);
        if (block) {
            offset = block->offset;
            pitch = block->pitch;
        }
#ifdef _MGRM_PROCESSES
    }else{ // for client
        REQ_HWSURFACE request = {surface->w, surface->h, surface->pitch, 0, NULL};
        REP_HWSURFACE reply = {0, 0, NULL};

        REQUEST req;

        req.id = REQID_HWSURFACE;
        req.data = &request;
        req.len_data = sizeof (REQ_HWSURFACE);

        ClientRequest (&req, &reply, sizeof (REP_HWSURFACE));

        block = (gal_vmblock_t *)reply.bucket;
        offset = reply.offset;
        pitch = reply.pitch;
#endif
    }

    if (block) {
        surface->hwdata = (struct private_hwdata *)calloc(1, sizeof(struct private_hwdata));
        surface->hwdata->vmblock = block;
        surface->hwdata->addr_phy = hidden->smem_start + offset;
        surface->hwdata->fd_g2d = hidden->fd_g2d;
		surface->hwdata->fd_disp = hidden->fd_disp;
        surface->pixels = hidden->pixels + offset;
        surface->pitch = pitch;
        memset (surface->pixels, 0, surface->pitch * surface->h);
        surface->flags |= GAL_HWSURFACE;
        return 0;
    }else{
        surface->flags &= ~GAL_HWSURFACE;
        return -1;
    }
}

static void SUNXI_FreeHWSurface(_THIS, GAL_Surface *surface)
{
    struct GAL_PrivateVideoData *hidden = this->hidden;
    struct private_hwdata *hwdata = surface->hwdata;

    if (hwdata->vmblock) {
        if (ISSERVER) {
            gal_vmbucket_free(&hidden->vmbucket, hwdata->vmblock);
#ifdef _MGRM_PROCESSES
        }else{
            REQ_HWSURFACE request = {surface->w, surface->h, surface->pitch, -1, surface->hwdata->vmblock};
            REP_HWSURFACE reply = {0, 0};

            REQUEST req;

            req.id = REQID_HWSURFACE;
            req.data = &request;
            req.len_data = sizeof (REQ_HWSURFACE);

            ClientRequest (&req, &reply, sizeof (REP_HWSURFACE));
#endif
        }
    }

    free(surface->hwdata);
    surface->hwdata = NULL;
    surface->pixels = NULL;
}

static int SUNXI_SetColors(_THIS, int firstcolor, int ncolors, GAL_Color *colors)
{
    /* do nothing of note. */
    return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another video routine -- notably UpdateRects.
*/
static void SUNXI_VideoQuit(_THIS)
{
    struct GAL_PrivateVideoData *data = this->hidden;
    memset(data->pixels, 0, data->smem_len);
    munmap(data->pixels, data->smem_len);
#if defined(SUNXI_MEM) || defined(ION_MEM)
	hwmem_virfree(addr_vir);
	hwmem_exit();
#endif
	if(data->fd_g2d >= 0) {
    	close(data->fd_g2d);
	}
    close(data->fd_fb);
	close(data->fd_disp);
}

#endif /* _MGGAL_SUNXI */
