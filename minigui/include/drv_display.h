#ifndef _DRV_DISPLAY_H
#define _DRV_DISPLAY_H

#define LAY_CNT 4
#define LAY_START 100
#define SEL 0
#define LAY_IDX(handle) ((handle)-LAY_START)
#define IDX_LAY(idx)	((idx)+LAY_START)
typedef enum
{
	FB_MODE_SCREEN0 		= 0,
	FB_MODE_SCREEN1 		= 1,
	FB_MODE_DUAL_SAME_SCREEN_TB	= 2,//two screen, top buffer for screen0, bottom buffer for screen1
    FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS = 3,//two screen, they have same contents;
}__fb_mode_t;

typedef enum
{
    	DISP_LAYER_WORK_MODE_NORMAL     = 0,    //normal work mode
        DISP_LAYER_WORK_MODE_PALETTE    = 1,    //palette work mode 
    	DISP_LAYER_WORK_MODE_INTER_BUF  = 2,    //internal frame buffer work mode
    	DISP_LAYER_WORK_MODE_GAMMA      = 3,    //gamma correction work mode
    	DISP_LAYER_WORK_MODE_SCALER     = 4,    //scaler work mode
}__disp_layer_work_mode_t;

typedef struct
{
	__fb_mode_t           			fb_mode;
	__disp_layer_work_mode_t    	mode;
	__u32                       	buffer_num;
	__u32                       	width;
	__u32                       	height;
	
	__u32                       	output_width;//used when scaler mode 
	__u32                       	output_height;//used when scaler mode
	
	__u32                       	primary_screen_id;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS
	__u32                       	aux_output_width;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS
	__u32                       	aux_output_height;//used when FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS
	
	//maybe not used anymore
	__u32                       	line_length;//in byte unit
	__u32                       	smem_len;
	__u32                       	ch1_offset;//use when PLANAR or UV_COMBINED mode
	__u32                       	ch2_offset;//use when PLANAR mode
}__disp_fb_create_para_t;

#define __bool signed char
typedef struct {__s32 x; __s32 y; __u32 width; __u32 height;}__disp_rect_t;
typedef struct {__u32 width;__u32 height;                   }__disp_rectsz_t;
typedef enum
{
        DISP_FORMAT_1BPP        = 0x0,
        DISP_FORMAT_2BPP        = 0x1,
        DISP_FORMAT_4BPP        = 0x2,
        DISP_FORMAT_8BPP        = 0x3,
        DISP_FORMAT_RGB655      = 0x4,
        DISP_FORMAT_RGB565      = 0x5,
        DISP_FORMAT_RGB556      = 0x6,
        DISP_FORMAT_ARGB1555    = 0x7,
        DISP_FORMAT_RGBA5551    = 0x8,
        DISP_FORMAT_ARGB888     = 0x9,//alpha padding to 0xff
        DISP_FORMAT_ARGB8888    = 0xa,
        DISP_FORMAT_RGB888      = 0xb,
        DISP_FORMAT_ARGB4444    = 0xc,

        DISP_FORMAT_YUV444      = 0x10,
        DISP_FORMAT_YUV422      = 0x11,
        DISP_FORMAT_YUV420      = 0x12,
        DISP_FORMAT_YUV411      = 0x13,
        DISP_FORMAT_CSIRGB      = 0x14,
}__disp_pixel_fmt_t;
typedef struct
{
	__s32 	id;
	__u32   addr[3];
	__u32   addr_right[3];//used when in frame packing 3d mode
	__bool  interlace;
	__bool  top_field_first;
	__u32   frame_rate; // *FRAME_RATE_BASE(靠靠1000)
	__u32   flag_addr;//dit maf flag address
	__u32   flag_stride;//dit maf flag line stride
	__bool  maf_valid;
	__bool  pre_frame_valid;
}__disp_video_fb_t;

typedef enum
{
//for interleave argb8888
        DISP_SEQ_ARGB                   = 0x0,//A在高位
        DISP_SEQ_BGRA                   = 0x2,
    
//for nterleaved yuv422
        DISP_SEQ_UYVY                   = 0x3,  
        DISP_SEQ_YUYV                   = 0x4,
        DISP_SEQ_VYUY                   = 0x5,
        DISP_SEQ_YVYU                   = 0x6,
    
//for interleaved yuv444
        DISP_SEQ_AYUV                   = 0x7,  
        DISP_SEQ_VUYA                   = 0x8,
    
//for uv_combined yuv420
        DISP_SEQ_UVUV                   = 0x9,  
        DISP_SEQ_VUVU                   = 0xa,
    
//for 16bpp rgb
        DISP_SEQ_P10                    = 0xd,//p1在高位
        DISP_SEQ_P01                    = 0xe,//p0在高位
    
//for planar format or 8bpp rgb
        DISP_SEQ_P3210                  = 0xf,//p3在高位
        DISP_SEQ_P0123                  = 0x10,//p0在高位
    
//for 4bpp rgb
        DISP_SEQ_P76543210              = 0x11,
        DISP_SEQ_P67452301              = 0x12,
        DISP_SEQ_P10325476              = 0x13,
        DISP_SEQ_P01234567              = 0x14,
    
//for 2bpp rgb
        DISP_SEQ_2BPP_BIG_BIG           = 0x15,//15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
        DISP_SEQ_2BPP_BIG_LITTER        = 0x16,//12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3
        DISP_SEQ_2BPP_LITTER_BIG        = 0x17,//3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12
        DISP_SEQ_2BPP_LITTER_LITTER     = 0x18,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    
//for 1bpp rgb
        DISP_SEQ_1BPP_BIG_BIG           = 0x19,//31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
        DISP_SEQ_1BPP_BIG_LITTER        = 0x1a,//24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7
        DISP_SEQ_1BPP_LITTER_BIG        = 0x1b,//7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24
        DISP_SEQ_1BPP_LITTER_LITTER     = 0x1c,//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
}__disp_pixel_seq_t;


typedef enum
{
        DISP_3D_OUT_MODE_CI_1 	= 0x5,//column interlaved 1
        DISP_3D_OUT_MODE_CI_2 	= 0x6,//column interlaved 2
        DISP_3D_OUT_MODE_CI_3 	= 0x7,//column interlaved 3
        DISP_3D_OUT_MODE_CI_4 	= 0x8,//column interlaved 4
        DISP_3D_OUT_MODE_LIRGB 	= 0x9,//line interleaved rgb

        DISP_3D_OUT_MODE_TB 	= 0x0,//top bottom
        DISP_3D_OUT_MODE_FP 	= 0x1,//frame packing
        DISP_3D_OUT_MODE_SSF 	= 0x2,//side by side full
        DISP_3D_OUT_MODE_SSH 	= 0x3,//side by side half
        DISP_3D_OUT_MODE_LI 	= 0x4,//line interleaved
        DISP_3D_OUT_MODE_FA 	= 0xa,//field alternative
}__disp_3d_out_mode_t;

typedef enum
{
        DISP_MOD_INTERLEAVED        = 0x1,   //interleaved,1个地址
        DISP_MOD_NON_MB_PLANAR      = 0x0,   //无宏块平面模式,3个地址,RGB/YUV每个channel分别存放
        DISP_MOD_NON_MB_UV_COMBINED = 0x2,   //无宏块UV打包模式,2个地址,Y和UV分别存放
        DISP_MOD_MB_PLANAR          = 0x4,   //宏块平面模式,3个地址,RGB/YUV每个channel分别存放
        DISP_MOD_MB_UV_COMBINED     = 0x6,   //宏块UV打包模式 ,2个地址,Y和UV分别存放
}__disp_pixel_mod_t;

typedef enum
{
        DISP_BT601  = 0,
        DISP_BT709  = 1,
        DISP_YCC    = 2,
        DISP_VXYCC  = 3,
}__disp_cs_mode_t;

typedef enum
{
        DISP_3D_SRC_MODE_TB 	= 0x0,//top bottom
        DISP_3D_SRC_MODE_FP 	= 0x1,//frame packing,left and right picture in separate address
        DISP_3D_SRC_MODE_SSF 	= 0x2,//side by side full
        DISP_3D_SRC_MODE_SSH 	= 0x3,//side by side half
        DISP_3D_SRC_MODE_LI 	= 0x4,//line interleaved
}__disp_3d_src_mode_t;

typedef struct
{
	__u32                   addr[3];    // frame buffer的内容地址，对于rgb类型，只有addr[0]有效
	__disp_rectsz_t         size;//单位是pixel
	__disp_pixel_fmt_t      format;
	__disp_pixel_seq_t      seq;
	__disp_pixel_mod_t      mode;
	__bool                  br_swap;    // blue red color swap flag, FALSE:RGB; TRUE:BGR,only used in rgb format
	__disp_cs_mode_t        cs_mode;    //color space 
	__bool                  b_trd_src; //if 3d source, used for scaler mode layer
	__disp_3d_src_mode_t    trd_mode; //source 3d mode, used for scaler mode layer
	__u32                   trd_right_addr[3];//used when in frame packing 3d mode
        __bool                  pre_multiply; //TRUE: pre-multiply fb
}__disp_fb_t;


typedef struct
{
	__disp_layer_work_mode_t		mode;       //layer work mode
	__bool                      	b_from_screen;
	__u8                        	pipe;       //layer pipe,0/1,if in scaler mode, scaler0 must be pipe0, scaler1 must be pipe1
	__u8                        	prio;       //layer priority,can get layer prio,but never set layer prio,从底至顶,优先级由低至高
	__bool                      	alpha_en;   //layer global alpha enable
	__u16                       	alpha_val;  //layer global alpha value 
	__bool                      	ck_enable;  //layer color key enable
	__disp_rect_t               	src_win;    // framebuffer source window,only care x,y if is not scaler mode
	__disp_rect_t               	scn_win;    // screen window
	__disp_fb_t                 	fb;         //framebuffer
	__bool                      	b_trd_out;  //if output 3d mode, used for scaler mode layer
	__disp_3d_out_mode_t        	out_trd_mode; //output 3d mode, used for scaler mode layer
}__disp_layer_info_t;


typedef enum tag_DISP_CMD
{
	//----disp global----
	DISP_CMD_SET_COLORKEY 			= 0x04,
	DISP_CMD_GET_COLORKEY 			= 0x04,
	//----layer----
	DISP_CMD_LAYER_REQUEST 			= 0x40,
	DISP_CMD_LAYER_RELEASE 			= 0x41,
	DISP_CMD_LAYER_OPEN 			= 0x42,
	DISP_CMD_LAYER_CLOSE 			= 0x43,
	DISP_CMD_LAYER_CK_ON 			= 0x51,
	DISP_CMD_LAYER_TOP 			= 0x56,
	DISP_CMD_LAYER_BOTTOM 			= 0x57,
	DISP_CMD_LAYER_SET_PARA 		= 0x4a,
	DISP_CMD_LAYER_GET_PARA 		= 0x4b,

	//----scaler----
	DISP_CMD_SCALER_REQUEST 		= 0x80,
	DISP_CMD_SCALER_RELEASE 		= 0x81,
	DISP_CMD_SCALER_EXECUTE 		= 0x82,
	DISP_CMD_SCALER_EXECUTE_EX 		= 0x83,
	
	//----framebuffer----
	DISP_CMD_FB_REQUEST 			= 0x280,
	DISP_CMD_FB_RELEASE 			= 0x281,
	DISP_CMD_FB_GET_PARA 			= 0x282,
	DISP_CMD_GET_DISP_INIT_PARA 	= 0x283,
	
	//----video----
	DISP_CMD_VIDEO_START 			= 0x100,
	DISP_CMD_VIDEO_STOP 			= 0x101,
	DISP_CMD_VIDEO_SET_FB 			= 0x102,
	DISP_CMD_VIDEO_GET_FRAME_ID 		= 0x103,
	DISP_CMD_VIDEO_GET_DIT_INFO 		= 0x104,	
	
}__disp_cmd_t;
#define FBIOGET_LAYER_HDL_0 0x4700

typedef struct
{
	__disp_fb_t   	input_fb;
	__disp_rect_t   source_regn;
	__disp_fb_t     output_fb;
	__disp_rect_t   out_regn;
}__disp_scaler_para_t;

/* possible overlay formats */
typedef enum vl_format 
{
    VL_FORMAT_MINVALUE     = 0x50,
    VL_FORMAT_RGBA_8888    = 0x51,
    VL_FORMAT_RGB_565      = 0x52,
    VL_FORMAT_BGRA_8888    = 0x53,
    VL_FORMAT_YCbYCr_422_I = 0x54,
    VL_FORMAT_CbYCrY_422_I = 0x55,
    VL_FORMAT_MBYUV420		= 0x56,
    VL_FORMAT_MBYUV422		= 0x57,
    VL_FORMAT_YUV420PLANAR	= 0x58,
    VL_FORMAT_YUV411PLANAR  = 0x59,
    VL_FORMAT_YUV420UVC     = 0x60,
    VL_FORMAT_DEFAULT      = 0x99,    // The actual color format is determined
    VL_FORMAT_MAXVALUE     = 0x100
}vl_format_t;
 
extern int disp_fd;
#define LAY_FRONT 0
#define LAY_BACK 1
typedef enum vl_cmd {
	VL_REQUEST = 1,
	VL_RELEASE = 2,
	VL_SETRECT = 3,
	VL_SETFRAMEPARA = 4,
	VL_GETCURFRAMEPARA = 5,
	VL_CLOSE = 6,
	VL_OPEN = 7,
	VL_ZORDER = 8,
	VL_SCALER=9,
}vl_cmd_t;

typedef struct {__u8  alpha;__u8 red;__u8 green; __u8 blue; }__disp_color_t;
typedef struct
{
	__disp_color_t   	ck_max;
	__disp_color_t   	ck_min;
	__u32             	red_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
	__u32             	green_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
	__u32             	blue_match_rule;//0/1:always match; 2:match if min<=color<=max; 3:match if color>max or color<min 
}__disp_colorkey_t;

typedef struct
{
    unsigned long   number;
    
    unsigned long   top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long   top_c;              // the address of frame buffer, which contains top field chrominance
    unsigned long   bottom_y;           // the address of frame buffer, which contains bottom field luminance
    unsigned long   bottom_c;           // the address of frame buffer, which contains bottom field chrominance

    signed char     bProgressiveSrc;    // Indicating the source is progressive or not
    signed char     bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    unsigned long   flag_addr;          //dit maf flag address
    unsigned long   flag_stride;        //dit maf flag line stride
    unsigned char   maf_valid;
    unsigned char   pre_frame_valid;
	unsigned int    handle;
}libhwclayerpara_t;

typedef struct
{
	__u32		w;
	__u32		h;
	__u32		format;
	__u32		hlay;
	HWND		hwnd;
}VideoInfo;

typedef struct
{
	vl_format_t fmt;
	__u32		w;
	__u32		h;
	__u32		addr_y;
	__u32		addr_c;
	
}VideoScalerIn;

#endif
