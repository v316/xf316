/* 
** $Id: init.c 13736 2011-03-04 06:57:33Z wanzheng $
**
** init.c: The Initialization/Termination routines for MiniGUI-Threads.
**
** Copyright (C) 2003 ~ 2008 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/11/05
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"

#ifndef __NOUNIX__
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/prctl.h>
#endif

#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "ial.h"
#include "internals.h"
#include "ctrlclass.h"
#include "cursor.h"
#include "event.h"
#include "misc.h"
#include "menu.h"
#include "timer.h"
#include "accelkey.h"
#include "clipboard.h"
#include "license.h"

int __mg_quiting_stage;

#ifdef _MGRM_THREADS
int __mg_enter_terminategui;

#ifdef FORBID_TWOCLICKS_FLING
int button1_count = 0;//鼠标点击(触摸屏按下)的次数
int button0_count = 0;//鼠标松开(触摸屏松开)的次数
#endif

/******************************* extern data *********************************/
extern void* DesktopMain (void* data);

#if 0
static void mouse_GestureDetector(PLWEVENT lwe)
{
    #define PI  (3.14)
    int i,j;
    int id;
    static MOUSEEVENT me_down[5]={{0},{0},{0},{0},{0}};  //保存5点按下坐标

    static int distX_old;     //双指X坐标差
    static int distY_old;     //双指Y坐标差，这两个用来判断旋转
    static int avgX_old;      //双指X坐标平均值
    static int avgY_old;      //双指Y坐标平均值，这两个用来判断双指平移
    static double length_old; //双指距离，用来判断放缩
    static double alpha_old;
    int distX_new;     //双指X坐标差
    int distY_new;     //双指Y坐标差，这两个用来判断旋转
    int avgX_new;      //双指X坐标平均值
    int avgY_new;      //双指Y坐标平均值，这两个用来判断双指平移
    double length_new; //双指距离，用来判断缩放
    double alpha_new;

    MSG Msg;
    Msg.hwnd = HWND_DESKTOP;

    id = lwe->data.me.id;
    me_down[id].id = id;
    me_down[id].x = lwe->data.me.x;
    me_down[id].y = lwe->data.me.y;
    //printf("--mouse_GestureDetector: id=%d \n",id);
    //printf("--mouse_GestureDetector: me_down[%d].x=%d \n",id,me_down[id].x);
    //printf("--mouse_GestureDetector: me_down[%d].y=%d \n",id,me_down[id].y);
    if(lwe->data.me.event == ME_LEFTDOWN)
    {
        me_down[id].event = lwe->data.me.event;
    }
    if(lwe->data.me.event == ME_LEFTUP)
    {
        me_down[id].event = lwe->data.me.event;
    }

    // 查找刚刚按下的前两点
    int No1=0,No2=0;
    for(i=0,j=0; i<5;i++)
    {
        if(me_down[i].event == ME_LEFTDOWN)
        {
            if(j==0)
                No1 = me_down[i].id;
            else if(j==1)
                No2 = me_down[i].id;
            j++;
        }
    }

    if( (j>1) && (No1 == id || No2 == id) && (lwe->data.me.event==ME_LEFTDOWN))
    {
        //识别出刚刚按下2点，计算出矢量初始值
        distX_old = ABS(me_down[No1].x - me_down[No2].x);
        distY_old = ABS(me_down[No1].y - me_down[No2].y);
        avgX_old = ABS(me_down[No1].x + me_down[No2].x)/2;
        avgY_old = ABS(me_down[No1].y + me_down[No2].y)/2;
        length_old = sqrt(distX_old*distX_old +distY_old*distY_old);
        alpha_old = acos(distX_old/length_old)*180/PI;
        //printf(" --mouse_GestureDetector: 111  length_old=%f, alpha_old=%f  \n",length_old,alpha_old);
    }
    else if( (j>1) && (No1 == id || No2 == id) && (lwe->data.me.event==ME_MOVED) )
    {
        //识别出按下2点后有一点在移动，计算出新矢量值
        distX_new = ABS(me_down[No1].x - me_down[No2].x);
        distY_new = ABS(me_down[No1].y - me_down[No2].y);
        avgX_new = ABS(me_down[No1].x + me_down[No2].x)/2;
        avgY_new = ABS(me_down[No1].y + me_down[No2].y)/2;
        length_new = sqrt(distX_new*distX_new +distY_new*distY_new);
        //printf("--mouse_GestureDetector: 222   length_new=%f, length_old=%f  \n",length_new,length_old);
        if( ABS(length_new-length_old)>=10)  //缩放的阈值是距离10
        {
            int zoom = (length_new/length_old)*1000;  // >1放大，<1缩小
            Msg.message = MSG_MOUSE_ZOOM;
            Msg.wParam = (WPARAM)zoom;
            Msg.lParam  = MAKELONG (me_down[id].x, me_down[id].y);
            QueueDeskMessage(&Msg);
        }
        alpha_new = acos(distX_new/length_new)*180/PI;
        //printf("alpha_new=%f, alpha_old=%f   \n", alpha_new,alpha_old);
        if (abs(alpha_new-alpha_old) >= 5)  //旋转的阈值是角度大于5°
        {
            int angle = alpha_new-alpha_old;  // <0顺时针旋转，>0逆时针旋转
            Msg.message = MSG_MOUSE_ROTATE;
            Msg.wParam = (WPARAM)angle;
            Msg.lParam  = MAKELONG (me_down[id].x, me_down[id].y);
            QueueDeskMessage(&Msg);
        }
    }

}
#endif

/************************* Entry of the thread of parsor *********************/
static void ParseEvent (PLWEVENT lwe)
{
    PMOUSEEVENT me;
    PKEYEVENT ke;
    MSG Msg;
    static int mouse_x = 0, mouse_y = 0;

    static unsigned int  scrolltime;  //scroll timer
    static int scrollX;
    static int scrollY;
    static int h_move_count = 0;
    static int v_move_count = 0;
    static int flingX;
    static int flingY;
    int deltX;
    int deltY;
    int direction=0;   // 4 directions
    int velocity=0;    //the speed

    ke = &(lwe->data.ke);
    me = &(lwe->data.me);
    Msg.hwnd = HWND_DESKTOP;
    Msg.wParam = 0;
    Msg.lParam = 0;

    Msg.time = __mg_timer_counter;

    if (lwe->type == LWETYPE_TIMEOUT) {
        Msg.message = MSG_TIMEOUT;
        Msg.wParam = (WPARAM)lwe->count;
        Msg.lParam = 0;
        QueueDeskMessage (&Msg);
    }
    else if (lwe->type == LWETYPE_KEY) {
        Msg.wParam = ke->scancode;
        Msg.lParam = ke->status;
        if(ke->event == KE_KEYDOWN){
            Msg.message = MSG_KEYDOWN;
        }
        else if (ke->event == KE_KEYUP) {
            Msg.message = MSG_KEYUP;
        }
        else if (ke->event == KE_KEYLONGPRESS) {
            Msg.message = MSG_KEYLONGPRESS;
        }
        else if (ke->event == KE_KEYALWAYSPRESS) {
            Msg.message = MSG_KEYALWAYSPRESS;
        }
        QueueDeskMessage (&Msg);
    }
    else if(lwe->type == LWETYPE_MOUSE) {
        Msg.wParam = me->status;
        Msg.lParam = MAKELONG (me->x, me->y);

        switch (me->event) {
        case ME_MOVED: {
            Msg.message = MSG_MOUSEMOVE;
            deltX = me->x - scrollX;
            deltY = me->y - scrollY;
            scrollX = me->x;   //record the coordinate
            scrollY = me->y;

            char *env = NULL;
            int move_adjust = 0;

            if ((env = getenv("MOSE_MOVE_ADJUST")) != NULL) {
                move_adjust = atoi(env);
            }

            if (move_adjust) {//don't process the horizontal move
                if(deltY>0)
                    direction = MOUSE_DOWN;
                else
                    direction = MOUSE_UP;
                velocity = (int)(0.06f * powf((abs(deltY) + 7.5f), 2) - 2.5f + 0.5f);
                Msg.wParam = MAKELONG (direction, velocity);
            } else {
                // 统计连续滑动过程中,　向某个方向滑动的次数
                if (abs(deltX) >= abs(deltY)) {
                    h_move_count++;
                } else {
                    v_move_count++;
                }

                //printf("x: %d, y: %d, h_move_count: %d, v_move_count: %d\n", abs(deltX), abs(deltY), h_move_count, v_move_count);
                if(h_move_count >= v_move_count) { // 水平方向的次数多,　则认为是水平滑动
                    v_move_count = 0;
                    if(deltX>0)
                        direction = MOUSE_RIGHT;
                    else
                        direction = MOUSE_LEFT;
                    velocity = (int)(0.06f * powf((abs(deltX) + 7.5f), 2) - 2.5f + 0.5f);
                } else { //scroll in the vertical //　垂直方向的次数多,　则认为是垂直滑动
                    h_move_count = 0;
                    if(deltY>0)
                        direction = MOUSE_DOWN;
                    else
                        direction = MOUSE_UP;
                    velocity = (int)(0.06f * powf((abs(deltY) + 7.5f), 2) - 2.5f + 0.5f);
                }
                /* char *env = NULL; */
                /* int adjust = 5; */
                /* if ((env = getenv("FLING_DELT")) != NULL) { */
                    /* adjust = atoi(env); */
                /* } */
                /* velocity = (abs(deltY)+adjust); */
                /* velocity = (int)(0.08f * powf((abs(deltY) + 7), 2) - 3 + 0.5f); */
                Msg.wParam = MAKELONG (direction, velocity);
            }
        }
            break;
        case ME_LEFTDOWN:
            Msg.message = MSG_LBUTTONDOWN;
            scrolltime = __mg_timer_counter;// start scroll timer
            scrollX = me->x;   //record the coordinate
            scrollY = me->y;
            flingX  = me->x;
            flingY  = me->y;
            break;
        case ME_LEFTUP:
            Msg.message = MSG_LBUTTONUP;
            h_move_count = 0;
            v_move_count = 0;
            break;
        case ME_LEFTDBLCLICK:
            Msg.message = MSG_LBUTTONDBLCLK;
            break;
        case ME_RIGHTDOWN:
            Msg.message = MSG_RBUTTONDOWN;
            break;
        case ME_RIGHTUP:
            Msg.message = MSG_RBUTTONUP;
            break;
        case ME_RIGHTDBLCLICK:
            Msg.message = MSG_RBUTTONDBLCLK;
            break;
        }
#if 0
		/* abadon the following */

        if (me->event != ME_MOVED && (mouse_x != me->x || mouse_y != me->y)) {
            int old = Msg.message;

            Msg.message = MSG_MOUSEMOVE;
            QueueDeskMessage (&Msg);
            Msg.message = old;

            mouse_x = me->x; mouse_y = me->y;
        }
#endif

        QueueDeskMessage (&Msg);
		/* Scroll Event: send message per 100ms when mouse is pressed. the arg are combined by direction and speed */
		if(Msg.message == MSG_MOUSEMOVE) {
			if(__mg_timer_counter >(scrolltime+10)) {
				// check direction and speed
				deltX = me->x - scrollX;
				deltY = me->y - scrollY;
				//OSA_DBG_MSG("GestureDetector:	deltX=%d, deltY=%d \n", deltX,deltY);
				scrolltime = __mg_timer_counter;
				scrollX = me->x;
				scrollY = me->y;

				if(abs(deltX) > abs(deltY))	{ //scroll in the horizontal
					velocity = abs(deltX);
					if(deltX>0)
					   direction = MOUSE_RIGHT;
					else
					   direction = MOUSE_LEFT;
				} else { //scroll in the vertical
					velocity = abs(deltY);
					if(deltY>0)
					   direction = MOUSE_DOWN;
					else
					   direction = MOUSE_UP;
				}
				Msg.message = MSG_MOUSE_SCROLL;
				Msg.wParam  = MAKELONG (direction, velocity);
				Msg.lParam  = MAKELONG (me->x, me->y);
				QueueDeskMessage (&Msg);
			}
		}
		/* Fling Event: send message when mouse is up. there is at least 20 pixel at one fling action , the arg is direction */
        //printf("me->x: %d, me->y: %d, flingX: %d, flingY: %d, button1_count: %d, button0_count: %d\n", me->x, me->y, flingX, flingY, button1_count, button0_count);
        if(Msg.message == MSG_LBUTTONUP) {
#ifdef FORBID_TWOCLICKS_FLING
            //鼠标点击(触摸屏按下)时只要没有松开收到的消息都是ME_LEFTDOWN，当真正fling时，会收到多次ME_LEFTDOWN，只有松开时才会收到一次ME_LEFTUP，
            //避免两只手同时点击屏幕时，由于ME_LEFTDOWN和ME_LEFTUP间的坐标差产生了MSG_MOUSE_FLING消息，采用以下策略。(>= 5可以根据实际情况具体调节)
            if ((button1_count - button0_count) >= 5) {
                deltX = me->x - flingX;
                deltY = me->y - flingY;
                if(abs(deltX)>20 || abs(deltY)>20) {
                    if(abs(deltX) > abs(deltY)){
                        velocity = abs(deltX);
                        if(deltX>0)
                           direction = MOUSE_RIGHT;
                        else
                           direction = MOUSE_LEFT;
                    } else {
                    velocity = abs(deltY);
                    if(deltY>0)
                       direction = MOUSE_DOWN;
                    else
                       direction = MOUSE_UP;
                    }
                    Msg.message = MSG_MOUSE_FLING;
                    Msg.wParam  = MAKELONG (direction, velocity);
                    Msg.lParam  = MAKELONG (me->x, me->y);
                    QueueDeskMessage (&Msg);
                }
            }
            button1_count = 0;
            button0_count = 0;
#else
            deltX = me->x - flingX;
            deltY = me->y - flingY;
            if(abs(deltX)>20 || abs(deltY)>20) {
                if(abs(deltX) > abs(deltY)){
                    velocity = abs(deltX);
                    if(deltX>0)
                       direction = MOUSE_RIGHT;
                    else
                       direction = MOUSE_LEFT;
                } else {
                velocity = abs(deltY);
                if(deltY>0)
                   direction = MOUSE_DOWN;
                else
                   direction = MOUSE_UP;
                }
                Msg.message = MSG_MOUSE_FLING;
                Msg.wParam  = MAKELONG (direction, velocity);
                Msg.lParam  = MAKELONG (me->x, me->y);
                QueueDeskMessage (&Msg);
            }
#endif
        }
		//mouse_GestureDetector(lwe);
	}
}


extern struct timeval __mg_event_timeout;

static void* EventLoop (void* data)
{
    LWEVENT lwe;
    int event;

    lwe.data.me.x = 0; lwe.data.me.y = 0;

    prctl(PR_SET_NAME, "GUIEventLoop", 0, 0, 0);

    sem_post ((sem_t*)data);

    while (__mg_quiting_stage > _MG_QUITING_STAGE_EVENT) {
        if (!IAL_WaitEvent) {
            printf("[file]:%s  [fun]:%s [line]:%d No Input!!!\n", __FILE__, __func__, __LINE__);
            return NULL;
        }

        event = IAL_WaitEvent (IAL_MOUSEEVENT | IAL_KEYEVENT, 0,
                        NULL, NULL, NULL, (void*)&__mg_event_timeout);
        if (event < 0) {
			usleep(10*1000); //reduce the CPU usage frequency
            continue;
        }

        lwe.status = 0L;

#ifdef _MGIAL_SUNXIKEYTSLIB
        if (event & IAL_MOUSEEVENT) {
            while (kernel_GetLWEvent (IAL_MOUSEEVENT, &lwe)) {
                ParseEvent (&lwe);
            }
        }
#else
        if (event & IAL_MOUSEEVENT && kernel_GetLWEvent (IAL_MOUSEEVENT, &lwe))
            ParseEvent (&lwe);
#endif

        lwe.status = 0L;
        if (event & IAL_KEYEVENT && kernel_GetLWEvent (IAL_KEYEVENT, &lwe))
            ParseEvent (&lwe);

        if (event == 0 && kernel_GetLWEvent (0, &lwe))
            ParseEvent (&lwe);
    }
    /* printf("Quit from EventLoop()\n"); */
    return NULL;
}

/************************** Thread Information  ******************************/

pthread_key_t __mg_threadinfo_key;

static inline BOOL createThreadInfoKey (void)
{
    if (pthread_key_create (&__mg_threadinfo_key, NULL))
        return FALSE;
    return TRUE;
}

static inline void deleteThreadInfoKey (void)
{
    pthread_key_delete (__mg_threadinfo_key);
}

MSGQUEUE* mg_InitMsgQueueThisThread (void)
{
    MSGQUEUE* pMsgQueue;

    if (!(pMsgQueue = malloc(sizeof(MSGQUEUE))) ) {
        return NULL;
    }

    if (!mg_InitMsgQueue(pMsgQueue, 0)) {
        free (pMsgQueue);
        return NULL;
    }

    pthread_setspecific (__mg_threadinfo_key, pMsgQueue);
    return pMsgQueue;
}

void mg_FreeMsgQueueThisThread (void)
{
    MSGQUEUE* pMsgQueue;

    pMsgQueue = pthread_getspecific (__mg_threadinfo_key);
#ifdef __VXWORKS__
    if (pMsgQueue != (void *)0 && pMsgQueue != (void *)-1) {
#else
    if (pMsgQueue) {
#endif
        mg_DestroyMsgQueue (pMsgQueue);
        free (pMsgQueue);
#ifdef __VXWORKS__
        pthread_setspecific (__mg_threadinfo_key, (void*)-1);
#else
        pthread_setspecific (__mg_threadinfo_key, NULL);
#endif
    }
}

/*
The following function is moved to src/include/internals.h as an inline 
function.
MSGQUEUE* GetMsgQueueThisThread (void)
{
    return (MSGQUEUE*) pthread_getspecific (__mg_threadinfo_key);
}
*/

/************************** System Initialization ****************************/

int __mg_timer_init (void);

static BOOL SystemThreads(void)
{
    sem_t wait;

    sem_init (&wait, 0, 0);

    // this is the thread for desktop window.
    // this thread should have a normal priority same as
    // other main window threads. 
    // if there is no main window can receive the low level events,
    // this desktop window is the only one can receive the events.
    // to close a MiniGUI application, we should close the desktop 
    // window.
#ifdef __NOUNIX__
    {
        pthread_attr_t new_attr;
        pthread_attr_init (&new_attr);
        pthread_attr_setstacksize (&new_attr, 16 * 1024);
        pthread_create (&__mg_desktop, &new_attr, DesktopMain, &wait);
        pthread_attr_destroy (&new_attr);
    }
#else
    pthread_create (&__mg_desktop, NULL, DesktopMain, &wait);
#endif

    sem_wait (&wait);

    __mg_timer_init ();

    // this thread collect low level event from outside,
    // if there is no event, this thread will suspend to wait a event.
    // the events maybe mouse event, keyboard event, or timeout event.
    //
    // this thread also parse low level event and translate it to message,
    // then post the message to the approriate message queue.
    // this thread should also have a higher priority.
    pthread_create (&__mg_parsor, NULL, EventLoop, &wait);
    sem_wait (&wait);

    sem_destroy (&wait);
    return TRUE;
}

BOOL GUIAPI ReinitDesktopEx (BOOL init_sys_text)
{
    return SendMessage (HWND_DESKTOP, MSG_REINITSESSION, init_sys_text, 0) == 0;
}

#ifndef __NOUNIX__
static struct termios savedtermio;

void* GUIAPI GetOriginalTermIO (void)
{
    return &savedtermio;
}

static void sig_handler (int v)
{
    if (v == SIGSEGV) {
        kill (getpid(), SIGABRT); /* cause core dump */
    }else if (__mg_quiting_stage > 0) {
        ExitGUISafely(-1);
    }else{
        exit(1); /* force to quit */
    }
}

static BOOL InstallSEGVHandler (void)
{
    struct sigaction siga;
    
    siga.sa_handler = sig_handler;
    siga.sa_flags = 0;
    
    memset (&siga.sa_mask, 0, sizeof (sigset_t));
    sigaction (SIGSEGV, &siga, NULL);
    sigaction (SIGTERM, &siga, NULL);
    sigaction (SIGINT, &siga, NULL);
    sigaction (SIGPIPE, &siga, NULL);

    return TRUE;
}
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

int GUIAPI InitGUI (int args, const char *agr[])
{
    int step = 0;

    __mg_quiting_stage = _MG_QUITING_STAGE_RUNNING;
    __mg_enter_terminategui = 0;

#ifdef HAVE_SETLOCALE
    setlocale (LC_ALL, "C");
#endif

#if defined (_USE_MUTEX_ON_SYSVIPC) || defined (_USE_SEM_ON_SYSVIPC)
    step++;
    if (_sysvipc_mutex_sem_init ())
        return step;
#endif

#ifndef __NOUNIX__
    // Save original termio
    tcgetattr (0, &savedtermio);
#endif

    /*initialize default window process*/
    __mg_def_proc[0] = PreDefMainWinProc;
    __mg_def_proc[1] = PreDefDialogProc;
    __mg_def_proc[2] = PreDefControlProc;

    step++;
    if (!mg_InitFixStr ()) {
        fprintf (stderr, "KERNEL>InitGUI: Init Fixed String module failure!\n");
        return step;
    }
    fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    step++;
    /* Init miscelleous*/
    if (!mg_InitMisc ()) {
        fprintf (stderr, "KERNEL>InitGUI: Initialization of misc things failure!\n");
        return step;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    step++;
    switch (mg_InitGAL ()) {
    case ERR_CONFIG_FILE:
        fprintf (stderr, 
            "KERNEL>InitGUI: Reading configuration failure!\n");
        return step;

    case ERR_NO_ENGINE:
        fprintf (stderr, 
            "KERNEL>InitGUI: No graphics engine defined!\n");
        return step;

    case ERR_NO_MATCH:
        fprintf (stderr, 
            "KERNEL>InitGUI: Can not get graphics engine information!\n");
        return step;

    case ERR_GFX_ENGINE:
        fprintf (stderr, 
            "KERNEL>InitGUI: Can not initialize graphics engine!\n");
        return step;
    }

#ifndef __NOUNIX__
    InstallSEGVHandler ();
#endif
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /*
     * Load system resource here.
     */
    step++;
    if (!mg_InitSystemRes ()) {
        fprintf (stderr, "KERNEL>InitGUI: Can not initialize system resource!\n");
        goto failure1;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /* Init GDI. */
    step++;
    if(!mg_InitGDI()) {
        fprintf (stderr, "KERNEL>InitGUI: Initialization of GDI failure!\n");
        goto failure1;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /* Init Master Screen DC here */
    step++;
    if (!mg_InitScreenDC (__gal_screen)) {
        fprintf (stderr, "KERNEL>InitGUI: Can not initialize screen DC!\n");
        goto failure1;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    g_rcScr.left = 0;
    g_rcScr.top = 0;
    g_rcScr.right = GetGDCapability (HDC_SCREEN_SYS, GDCAP_MAXX) + 1;
    g_rcScr.bottom = GetGDCapability (HDC_SCREEN_SYS, GDCAP_MAXY) + 1;
    license_create();
    splash_draw_framework(); 
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /* Init mouse cursor. */
    step++;
    if( !mg_InitCursor() ) {
        fprintf (stderr, "KERNEL>InitGUI: Count not init mouse cursor!\n");
        goto failure1;
    }
    splash_progress();
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /* Init low level event */
    step++;
    if(!mg_InitLWEvent()) {
        fprintf(stderr, "KERNEL>InitGUI: Low level event initialization failure!\n");
        goto failure1;
    }
    splash_progress();
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /** Init LF Manager */
    step++;
    if (!mg_InitLFManager ()) {
        fprintf (stderr, "KERNEL>InitGUI: Initialization of LF Manager failure!\n");
        goto failure;
    }
    splash_progress();
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
#ifdef _MGHAVE_MENU
    /* Init menu */
    step++;
    if (!mg_InitMenu ()) {
        fprintf (stderr, "KERNEL>InitGUI: Init Menu module failure!\n");
        goto failure;
    }
#endif
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    /* Init control class */
    step++;
    if(!mg_InitControlClass()) {
        fprintf(stderr, "KERNEL>InitGUI: Init Control Class failure!\n");
        goto failure;
    }
    splash_progress();
#ifndef SUNXI_MIN
    /* Init accelerator */
    step++;
    if(!mg_InitAccel()) {
        fprintf(stderr, "KERNEL>InitGUI: Init Accelerator failure!\n");
        goto failure;
    }
    splash_progress();
#endif
    step++;
    if (!mg_InitDesktop ()) {
        fprintf (stderr, "KERNEL>InitGUI: Init Desktop failure!\n");
        goto failure;
    }
    splash_progress();
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    step++;
    if (!mg_InitFreeQMSGList ()) {
        fprintf (stderr, "KERNEL>InitGUI: Init free QMSG list failure!\n");
        goto failure;
    }
    splash_progress();
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    step++;
    if (!createThreadInfoKey ()) {
        fprintf (stderr, "KERNEL>InitGUI: Init thread hash table failure!\n");
        goto failure;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    splash_delay();

    step++;
    if (!SystemThreads()) {
        fprintf (stderr, "KERNEL>InitGUI: Init system threads failure!\n");
        goto failure;
    }
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    SetKeyboardLayout ("default");

    SetCursor (GetSystemCursor (IDC_ARROW));

    SetCursorPos (g_rcScr.right >> 1, g_rcScr.bottom >> 1);
    //fprintf(stderr, "---%s %d---\n", __FUNCTION__, __LINE__);
    mg_TerminateMgEtc ();

    return 0;

failure:
    mg_TerminateLWEvent ();
failure1:
    mg_TerminateGAL ();
    fprintf (stderr, "KERNEL>InitGUI: Init failure, please check "
            "your MiniGUI configuration or resource.\n");
    return step;
}

void GUIAPI TerminateGUI (int not_used)
{
    /* printf("Quit from MiniGUIMain()\n"); */

    __mg_enter_terminategui = 1;

    pthread_join (__mg_desktop, NULL);

    /* DesktopProc() will set __mg_quiting_stage to _MG_QUITING_STAGE_EVENT */
    /* hack by yinzh */
    pthread_cancel (__mg_parsor);

    deleteThreadInfoKey ();
    license_destroy();

    __mg_quiting_stage = _MG_QUITING_STAGE_TIMER;
    mg_TerminateTimer ();

    mg_TerminateDesktop ();

    mg_DestroyFreeQMSGList ();
#ifndef SUNXI_MIN
    mg_TerminateAccel ();
#endif
    mg_TerminateControlClass ();
#ifdef _MGHAVE_MENU
    mg_TerminateMenu ();
#endif
    mg_TerminateMisc ();
    mg_TerminateFixStr ();

#ifdef _MGHAVE_CURSOR
    mg_TerminateCursor ();
#endif
    mg_TerminateLWEvent ();

    mg_TerminateScreenDC ();
    mg_TerminateGDI ();
    mg_TerminateLFManager ();
    mg_TerminateGAL ();
#ifndef SUNXI_MIN
    extern void mg_miFreeArcCache (void);
    mg_miFreeArcCache ();
#endif
    /* 
     * Restore original termio
     *tcsetattr (0, TCSAFLUSH, &savedtermio);
     */

#if defined (_USE_MUTEX_ON_SYSVIPC) || defined (_USE_SEM_ON_SYSVIPC)
    _sysvipc_mutex_sem_term ();
#endif
}

#endif /* _MGRM_THREADS */

void GUIAPI ExitGUISafely (int exitcode)
{
#ifdef _MGRM_PROCESSES
    if (mgIsServer)
#endif
    {
#   define IDM_CLOSEALLWIN (MINID_RESERVED + 1) /* see src/kernel/desktop-*.c */
        SendNotifyMessage(HWND_DESKTOP, MSG_COMMAND, IDM_CLOSEALLWIN, 0);
        SendNotifyMessage(HWND_DESKTOP, MSG_ENDSESSION, 0, 0);

        if (__mg_quiting_stage > 0) {
            __mg_quiting_stage = _MG_QUITING_STAGE_START;
        }
    }
}
