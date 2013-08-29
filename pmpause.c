/* PMPAUSE -- warns of long periods of typing/mouse use, and hints that
 *            a pause might be desireable.
 *
 * Written by Ewen McNeill <ewen@naos.co.nz>, Dec 1997.  
 * Ideas based on Xpause and Xpause2 by Julian Anderson, and Paul Thomas.
 * Copyright Ewen McNeill, 1997-1998.  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *---------------------------------------------------------------------------
 *
 * It's often been said that you have to build things twice: once to build
 * it, and once to build it right.  This is the first attempt.  This version
 * is fairly limited, and its design is perhaps sub optimal but it does work
 * and provides a basis for further development/re-design.
 *
 * NOTE: OS/2 PM programs do have valid stdout/stderr streams which can
 * be used, but by default the output is not displayed.  It is possible
 * to display this output by piping it to another program (eg, less, more,
 * cat).  For this reason if the usage information (eg error in options),
 * then a beep is made before outputing the text.  Ideally a window should
 * pop up with the information, but that'll have to wait for a later version :-)
 *
 *---------------------------------------------------------------------------
 * $Id: pmpause.c 0.16 1998/01/13 02:41:32 ewen Exp $
 */

/* OS/2 Header portions required */
#define INCL_WIN
#define INCL_GPI
#define INCL_DOSPROCESS
#define INCL_DOSMODULEMGR
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "pmpause.h"

/* Prototypes */
MRESULT EXPENTRY window_handler(HWND handle, ULONG msg, MPARAM p1, MPARAM p2);
LONG decide_colour(int level);
void usage(void);
VOID APIENTRY ExeTrap(void);

/* Name of window class */
static unsigned char winClass[] = "pause";

/* Exit codes */
#define EXIT_OK          ( 0)
#define EXIT_INIT_ERROR  (90)
#define EXIT_USAGE       (99)

/* Colours, and thresholds for their use */
#define COLOUR_NORMAL    (CLR_GREEN)
#define COLOUR_NOTIFY    (CLR_YELLOW)
#define COLOUR_WARN      (CLR_RED)
#define COLOUR_BKGND     (CLR_BACKGROUND)

/* Message numbers to use internally     */
#define MSG_GO_UP        (WM_USER + 1)
#define MSG_GO_DOWN      (WM_USER + 2)

/* Runtime configurable information: global, static, to allow view from
 * all functions (alternative for later: window words?)
 */

/* Minimum and maximum levels   */
static int min_level        =   0;
static int max_level        = 180;

/* Transitition levels          */
static int threshold_notify =  90;
static int threshold_warn   = 150;

/* Step amounts                 */
static int step_up          =   4;
static int step_down        =  12;

/* Orientation -- horizontal or vertical (vertical if not horizontal) */
static int horizontal       =FALSE;

/* How often to check if there has been activity recently */
static int check_seconds    =   4;

/* Width and Height (can be overridden from command line) */
static long width           =  10;
static long height          = 234;

/* Position of window:
 * Without -x:  positive relative to bottom, left,
 *              negative relative to top, right
 * With    -x:  positive relative to top, left,
 *              negative relative to bottom, right
 */
static long xoffset         =   0;
static int  xoffsetneg      =TRUE;
static long yoffset         =   0;
static int yoffsetneg       =FALSE;

static int reltopleft       =FALSE;

/* Beep when reaching top level (only on way up, and if stepped up)        */
static int beepattop        =FALSE;

#define ARERT_BEEP_FREQ    (1440)      /* Hz */
#define ALERT_BEEP_LEN     (  10)      /* ms */

/* Anchor block and message queue, global because several things need them */
static HAB   ab       = NULLHANDLE;    /* Anchor block   */ 
static HMQ   mq       = NULLHANDLE;    /* Message queue  */

/* Define to use the DLL routines to hook the message queue */
#define USE_DLL

/*========================================================================*/

int main(int argc, char **argv)
{
  QMSG  msg;                      /* Window message */
  ULONG winFlags = 0UL;           /* Window flags   */
  HWND  frame    = NULLHANDLE;    /* Window frame   */
  HWND  client   = NULLHANDLE;    /* Window client  */
  HMODULE hmDll  = NULLHANDLE;    /* Handle to our own module (DLL) */
  char  **args   = NULL;          /* To parse command line arguments */
  int   show_usage = 0;           /* show usage & exit if set to 1 */
  long  sx = 0L, sy = 0L;         /* Screen x, y maximums */

  /* Parse the command line arguments */
  /* Ideally something like getopt() would be used here, but at present
   * a hand coded approach is just slightly easier.
   */
  args = (argv + 1);
  while(*args != NULL)
  {
    const char *arg = *args;
  
    if (*arg == '-')
    {
      while(*(++arg) != '\0')
        switch(*arg)
        {
        case 'h':                   /* Horizontal meter */
          horizontal = TRUE;
          break;
        case 'v':                   /* Vertical meter */
	  horizontal = FALSE;
          break;
        case 'x':                   /* Relative to top left, like X */
          reltopleft = TRUE;
          break;
        case 'b':                   /* Beep on reaching top level */
          beepattop = TRUE;
          break;
        case 'm':                   /* Maximum value */
          if (*(args + 1) != NULL)
            max_level = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case 'n':                   /* Notify level */
          if (*(args + 1) != NULL)
            threshold_notify = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case 'w':                   /* Warn level */
          if (*(args + 1) != NULL)
            threshold_warn = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case 'u':                   /* Step up amount */
          if (*(args + 1) != NULL)
            step_up = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case 'd':                   /* Step down amount */
          if (*(args + 1) != NULL)
            step_down = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case 'c':                   /* Check frequency */
          if (*(args + 1) != NULL)
            check_seconds = atoi(*(++args));
          else
            show_usage = 1;         /* Insufficient arguments */
          break;
        case '?':
          /* FALL THROUGH */
        default:
          show_usage = 1;           /* Unknown argument */
        break;
      }
    } /* Started with "-" */
    else if (isdigit(*arg))
    { /* This looks like it might be a position string, so try to parse it   */
      long w = 0L, h = 0L, x = 0L, y = 0L;      /* Width, Height, xoff, yoff */
      char xp = ' ', yp = ' ';                  /* X off positive, y off pos */
      int rc = 0;

      rc = sscanf(arg, "%ldx%ld%1c%ld%1c%ld", &w, &h, &xp, &x, &yp, &y);

      if (rc == 6)
      { /* Scanned all the arguments that we wanted, use them                */
        width      = w;             height     = h;
        xoffset    = x;             yoffset    = y;
        xoffsetneg = (xp == '-');   yoffsetneg = (yp == '-');

        fprintf(stderr, "Got: width = %ld, height = %ld, "
                        "xoffset = %c%ld, yoffset = %c%ld\n",
                width, height, (xoffsetneg ? '-' : '+'), xoffset,
                               (yoffsetneg ? '-' : '+'), yoffset);
        fflush(stderr);
      }
      else if (rc == 2)
      { /* Got two arguments, so just width and height                       */
        width      = w;             height     = h;
      }
      else
        show_usage = 1;
    }
    else /* Unexpected argument */
      show_usage = 1;
 
    args++;
  }

  if (show_usage ||               /* Invalid arguments/requested help */
                                  /* Sanity checks: */
      (! (min_level < max_level))                                           ||
      (! (min_level < threshold_notify && threshold_notify < max_level))    ||
      (! (threshold_notify < threshold_warn && threshold_warn < max_level)) ||
      (! (step_up   <= (max_level - min_level)))                            ||
      (! (step_down <= (max_level - min_level)))                            ||
      (! ((width > 0) && (height > 0))))
  {
    usage();
    exit(EXIT_USAGE);
  }

  /* Figure out how big the screen is, so we can decide on positions      */
  sx = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
  sy = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
  fprintf(stderr, "Window size is %ld x %ld\n", sx, sy); fflush(stderr);

  /* Position the window -- two positionings are applicable:
   *   OS/2 style:  positive relative bottom, left; negative top, right
   *   X style:     positive relative top, left; negative bottom, right
   */
  if (reltopleft)
  { /* X style */
    if (xoffsetneg)
      xoffset = (sx - xoffset - width);

    /* Need to flip Y axis.  For negative values this means we need:
     * - negative value specifies bottom edge, relative to bottom, which
     *   is exactly the OS/2 semantics, so we do nothing
     * - positive value specifies top edge, relative to top, which means
     *   we need to flip the values over.
     */
    if (! yoffsetneg)
      yoffset = (sy - yoffset - height);
  }
  else
  { /* OS/2 style */
    if (xoffsetneg)
      xoffset = (sx - xoffset - width);

    if (yoffsetneg)
      yoffset = (sy - yoffset - height);   
  }

  xoffsetneg = FALSE; yoffsetneg = FALSE;  /* They're not negative any longer*/

  fprintf(stderr, "Window to be created at: %ld, %ld, of size %ld x %ld\n",
          xoffset, yoffset, width, height);
  fflush(stderr);
  
  /* Frame contents -- we want a little border, and to be on the tasklist,
   *                   but we don't want any buttons or things. 
   */
  winFlags = FCF_BORDER | FCF_TASKLIST | FCF_NOBYTEALIGN | FCF_AUTOICON;

  /* Get anchor block, and message queue ---------------------------------*/
  ab = WinInitialize(0);
  if (ab == NULLHANDLE)
    exit(EXIT_INIT_ERROR);

  mq = WinCreateMsgQueue(ab, 0);
  if (mq == NULLHANDLE)
  {
    WinTerminate(ab);
    exit(EXIT_INIT_ERROR);
  }

  /* Trap exceptions so we can release our hook */
  DosExitList(EXLST_ADD, (PFNEXITLIST)ExeTrap);

  /* Register window class -----------------------------------------------*/
  if (!WinRegisterClass(ab, (PSZ)winClass, (PFNWP)window_handler, 
			CS_SIZEREDRAW | CS_SYNCPAINT, 0))
  {
    WinDestroyMsgQueue(mq);
    WinTerminate(ab);
    exit(EXIT_INIT_ERROR);
  }

  /* Create window -------------------------------------------------------*/

  /* Creates a window of our class, sizing/visibility done later */
  frame = WinCreateStdWindow(HWND_DESKTOP, 0, &winFlags,
			     (PSZ)winClass, (PSZ)"PM Pause", 0,
			     0, 0, &client);
  if (frame == NULLHANDLE)
  {
    WinDestroyMsgQueue(mq);
    WinTerminate(ab);
    exit(EXIT_INIT_ERROR);
  }

  /* Now position the window, and make it visible */
#ifdef DEBUG  
  fprintf(stderr, "Calling WinSetWindowPos(..., %ld, %ld, %ld, %ld,....)\n",
          xoffset, yoffset, width, height); fflush(stderr);
#endif          
  if (WinSetWindowPos(frame, HWND_TOP, xoffset, yoffset, width, height,
                        (SWP_SIZE|SWP_MOVE|SWP_ZORDER|SWP_SHOW)))
  {
#ifdef DEBUG  	
    fprintf(stderr, "Set window position.\n");
    fflush(stderr);
#endif
  }
  else
  {
#ifdef DEBUG
    fprintf(stderr, "Window position set failed (%lx).\n", WinGetLastError(ab));
    fflush(stderr);
#endif    
  }

#ifdef USE_DLL  
  /* Hook the input queue */
  if (PMPauseInit(client))
  {
  #ifdef DEBUG
    fprintf(stderr, "Hooked message queue.\n"); fflush(stderr);
  #endif
  }
#endif  

  /* Message Loop --------------------------------------------------------*/
  while(WinGetMsg(ab, &msg, 0, 0, 0))
    WinDispatchMsg(ab, &msg);

  /* Cleanup -------------------------------------------------------------*/
  /* NB: Window Class is deregistered when program exits.                 */

#ifdef USE_DLL  
  PMPauseKill();
#endif  
  WinDestroyWindow(frame);
  WinDestroyMsgQueue(mq);
  WinTerminate(ab);
  
#ifdef USE_DLL
  /* Last act -- we free the module */
  if(! DosQueryModuleHandle(DLLNAME, &hmDll))
    DosFreeModule(hmDll);            /* Free DLL, to force release of mem */
#endif    

  return(EXIT_INIT_ERROR);
}

/*========================================================================*/

/* Pause window -- presents a thermometer style display that goes up and 
 * down as requested, changing colours when it is over particular thresholds
 */

MRESULT EXPENTRY window_handler(HWND handle, ULONG msg, MPARAM p1, MPARAM p2)
{
  static int level = 0;       /* Level of meter */
  static unsigned long timer = 0UL;    /* Number of our timer                */

  switch(msg)
  {
  case WM_CHAR:               /* Keyboard characters, we respond to arrows   */
    {
      int processed = 0;      /* Flag that message has been processed        */
      
      if (SHORT1FROMMP(p1) & KC_KEYUP)
        return WinDefWindowProc(handle, msg, p1, p2);  /* Ignore releases    */
      else
      {
        switch(SHORT2FROMMP(p2))/* Updates done via messages to ourselves for*/
        {                       /* ease of extension to other triggers.      */
        case VK_LEFT:           /* Step down if horizontal*/
          if (horizontal)       WinSendMsg(handle, MSG_GO_DOWN, 0, 0);
          processed = 1;
          break;

        case VK_RIGHT:          /* Step up if horizontal */
          if (horizontal)       WinSendMsg(handle, MSG_GO_UP,   0, 0);
          processed = 1;
          break;

        case VK_DOWN:           /* Step down if vertical */
          if (! horizontal)     WinSendMsg(handle, MSG_GO_DOWN, 0, 0);
          processed = 1;
          break;

        case VK_UP:             /* Step up if vertical */
          if (! horizontal)     WinSendMsg(handle, MSG_GO_UP,   0, 0);
          processed = 1;
          break;

        default:
          break;
        } /* Switch (virtual key) */

        /* Check for a quit keystroke; if found post ourselves quit msg */
        if ((SHORT1FROMMP(p1) & KC_CHAR) &&
            (SHORT1FROMMP(p2) == 'q' ||
             SHORT1FROMMP(p2) == 'Q'))
          WinPostMsg(handle, WM_CLOSE, 0, 0);
        else
          if (! processed)
            return WinDefWindowProc(handle, msg, p1, p2);
      } /* Keydown */
    } /* Keystroke */
    break;

  case MSG_GO_UP:           /* Handle updating the meter up/down together */
  case MSG_GO_DOWN:
    {
      RECTL  updrect;         /* Update rectangle */
      int    oldlevel = level;/* Old level value */

      /* Adjust level depending on message (note: relies on being able to
       * overstep the value and correct it later)
       */
      if (msg == MSG_GO_UP)   level += step_up;
      else                    level -= step_down;

      if (level > max_level)  level = max_level;
      if (level < min_level)  level = min_level;
        
#if defined(DEBUG)
      fprintf(stderr, "Level changed: %d -> %d\n", oldlevel, level);
#endif
      if (level == max_level && (oldlevel < max_level) && beepattop)
      { /* We've reached the top level with this move, so we should beep  */
        DosBeep(ARERT_BEEP_FREQ, ALERT_BEEP_LEN);
      }

      /* If the value changed, we need to figure out what to update       */
      /* Then invalidate that rectangle, for repainting                   */
      /* Repainting done by general paint code for simplicity.            */
      if (level != oldlevel)
      {
	LONG oldcolour;
	LONG newcolour;

	WinQueryWindowRect(handle, &updrect);   /* Get whole window size  */
#if defined(DEBUG)
	fprintf(stderr, "Window is: %ld, %ld, %ld, %ld\n",
		updrect.xLeft, updrect.xRight, updrect.yBottom, updrect.yTop);
	fflush(stderr);
#endif

	/* Figure out colours that would have been used (to detect crossing
         * a boundary)
         */
	oldcolour = decide_colour(oldlevel);
	newcolour = decide_colour(level);

	if (horizontal)
	{ /* horizontal window, we want to set X values */
	  LONG old, new;
	  old = (((updrect.xRight - updrect.xLeft) * (LONG)oldlevel)
		 / (LONG)max_level);
	  new = (((updrect.xRight - updrect.xLeft) * (LONG)level)
		 / (LONG)max_level);

	  if (level > oldlevel)
	  { /* Level is going up, want fraction from oldlevel to new level */
            /* Unless colour changed, then want up to new level */
	    if (oldcolour == newcolour)
	      updrect.xLeft  = old;

	    updrect.xRight = new;
	  }
	  else
	  { /* Level is going down, want fraction from new level to old    */
            /* Unless colour changed, then want up to new level */
	    if (oldcolour == newcolour)
	      updrect.xLeft  = new;

	    updrect.xRight = (old+1);
	  }
	}
	else 
	{ /* Vertical window, we want to set Y values */
	  LONG old, new;
	  old = (((updrect.yTop - updrect.yBottom) * (LONG)oldlevel)
		 / (LONG)max_level);
	  new = (((updrect.yTop - updrect.yBottom) * (LONG)level)
		 / (LONG)max_level);

	  if (level > oldlevel)
	  { /* Level is going up, want fraction from oldlevel to new level */
            /* Unless colour changed, then want up to new level */
	    if (oldcolour == newcolour)
	      updrect.yBottom = old;

	    updrect.yTop    = new;
	  }
	  else
	  { /* Level is going down, want fraction from new level to old    */
            /* Unless colour changed, then want up to new level */
	    if (oldcolour == newcolour)
	      updrect.yBottom = new;

	    updrect.yTop    = (old+1);
	  }
	}

#if defined(DEBUG)
	fprintf(stderr, "Triggering update of: %ld, %ld, %ld, %ld\n",
		updrect.xLeft, updrect.xRight, updrect.yBottom, updrect.yTop);
	fflush(stderr);
#endif
	WinInvalidateRect(handle, &updrect, FALSE);
      }
    } /* Meter adjusted */
    break;

  case WM_PAINT:              /* Paint portions of the window */
    {
      RECTL   updrect;        /* Rectangle to update */
      RECTL   window;         /* Window rectangle */
      RECTL   box;            /* Rectangle to draw into */
      HPS     ps = NULLHANDLE;/* Presentation space  */
      LONG    fgcolour = CLR_WHITE;
            
      WinQueryWindowRect(handle, &window);   /* Get whole window size  */

      ps = WinBeginPaint(handle, NULLHANDLE, &updrect);
      if (ps == NULLHANDLE)
      { /* Oh dear, we can't get a handle to write to the window */
	DosBeep(600,50);
	break;
      }

#if defined(DEBUG)
      fprintf(stderr, "Update triggered for: %ld, %ld, %ld, %ld\n",
	      updrect.xLeft, updrect.xRight, updrect.yBottom, updrect.yTop);
      fflush(stderr);
#endif

      /* Figure out the colour to use when filling rectangle */
      fgcolour = decide_colour(level);

      /* Need to fill in two bits: from start to level, from level to top */
      /* NOTE: WinFillRect will paint into the left/bottom edges, but not */
      /* into the top/right edges, unless left=right, or bottom=top. Thus */
      /* we do not need to add an offset between fill/empty in, because it*/
      /* will be done for us automatically.  The only exception is filling*/
      /* strips one pixel wide, and that shouldn't happen too often.      */
      if (horizontal)
      { /* horizontal window, fill across */
	LONG levelpos;

	/* Figure out the middle point */
	levelpos = (((window.xRight - window.xLeft) *  (LONG)level) 
		    / (LONG)max_level);	
	if (levelpos == 0)
	  levelpos = -1;        /* Hack: so that it gets completely empty */

#if defined(DEBUG)
	fprintf(stderr, "Division is at: %ld\n", levelpos);
	fflush(stderr);
#endif

	/* Update the foreground colour bit as required */
	if (levelpos >= updrect.xLeft && levelpos <= updrect.xRight)
	{
	  box.xLeft   = updrect.xLeft;   box.xRight = levelpos;
	  box.yBottom = updrect.yBottom; box.yTop   = updrect.yTop;
#if defined(DEBUG)
	  fprintf(stderr, "Filling: %ld, %ld, %ld, %ld\n",
		  box.xLeft,box.xRight, box.yBottom, box.yTop);
	  fflush(stderr);
#endif
	  if (WinFillRect(ps, &box, fgcolour) != TRUE)
	  {
	    DosBeep(1200,50);
#ifdef DEBUG	    
	    fprintf(stderr, "*ERROR*, %08lx\n", WinGetLastError(ab)); 
	    fflush(stderr);
#endif	    
	  }
	}

	/* Update the background colour bit as required */
	if (updrect.xRight >= levelpos)
	{
	  box.xLeft   = levelpos;        box.xRight = updrect.xRight;
	  box.yBottom = updrect.yBottom; box.yTop   = updrect.yTop;
#if defined(DEBUG)
	  fprintf(stderr, "Emptying: %ld, %ld, %ld, %ld\n",
		  box.xLeft,box.xRight, box.yBottom, box.yTop);
	  fflush(stderr);
#endif
	  if (WinFillRect(ps, &box, COLOUR_BKGND) != TRUE)
	  {
	    DosBeep(1300,100);
#ifdef DEBUG	    
	    fprintf(stderr, "<ERROR>, %08lx\n", WinGetLastError(ab));
	    fflush(stderr);
#endif	    
	  }
	}
      }
      else 
      { /* Vertical window, fill up */
	LONG levelpos;

	/* Figure out the middle point */
	levelpos = (((window.yTop - window.yBottom) * (LONG)level)
		    / (LONG)max_level);	
	if (levelpos == 0)
	  levelpos = -1;        /* Hack: so that it gets completely empty */

	/* Update the foreground colour bit as required */
	if (levelpos >= updrect.yBottom && levelpos <= updrect.yTop)
	{
	  box.xLeft   = updrect.xLeft;   box.xRight = updrect.xRight;
	  box.yBottom = updrect.yBottom; box.yTop   = levelpos;
	  WinFillRect(ps, &box, fgcolour);
	}

	/* Update the background colour bit as required */
	if (updrect.yTop >= levelpos)
	{
	  box.xLeft   = updrect.xLeft;   box.xRight = updrect.xRight;
	  box.yBottom = levelpos;        box.yTop   = updrect.yTop;
	  WinFillRect(ps, &box, COLOUR_BKGND);
	}
      }

      WinEndPaint(ps);        /* Release presentation space                 */
    }

    break;

  case WM_CREATE:             /* Create window -- we use this to start our  */
                              /* timer up.                                  */
                              /* NOTE: I _hope_ this timer goes away when   */
                              /* our function ends.  I can only see a way   */
                              /* to stop the timer, not to destroy it.      */
     timer = WinStartTimer(ab, handle, 1UL, check_seconds * 1000UL);
#ifdef DEBUG
     fprintf(stderr, "Started timer every %ld seconds (# %ld)\n",
                     (check_seconds * 1000UL), timer);
     fflush(stderr);                     
#endif

     return (MRESULT)FALSE;   /* Continue window creation, we're done here. */
     break;

  case WM_CLOSE:              /* Close the window down                      */
     if (WinStopTimer(ab, handle, 1UL))
     {
#ifdef DEBUG
       fprintf(stderr, "Stopped timer\n"); fflush(stderr);
#endif     	
     }
     
     WinPostMsg(handle, WM_QUIT, 0, 0);   /* Now we've cleaned up, can quit */
     return (MRESULT)FALSE;
     break;

  case WM_TIMER:              /* Timer triggered                            */
     if (SHORT1FROMMP(p1) == timer)
     { /* It's our timer, we can do our stuff.                              */
#ifdef USE_DLL
       if (PMPauseGetCount() > 0)
       { /* Yes, something has happened recently                            */
       	 PMPauseClearCount();
       	 WinPostMsg(handle, MSG_GO_UP, 0, 0);
       }
       else
       { /* No, all quiet on the home front                                 */
         WinPostMsg(handle, MSG_GO_DOWN, 0, 0);
       }
#endif       
#ifdef DEBUG
       fprintf(stderr, "Timer tick received.\n");
       fflush(stderr);
#endif       
       return (MRESULT)FALSE;
     }
     else                     /* Someone else's timer, pass it on to default*/
       return WinDefWindowProc(handle, msg, p1, p2);
     break;
  
  case WM_ERASEBACKGROUND:    /* Yes, we would like our background erased   */
    fprintf(stderr, "Asked for background to be erased\n"); fflush(stderr);
    return (MRESULT)TRUE;

  default:
    return WinDefWindowProc(handle, msg, p1, p2);
  } 

  return (MRESULT)0;
}

/* Figure out what colour to use for a given level */
LONG decide_colour(int level)
{
  if      (level < threshold_notify)
    return COLOUR_NORMAL;
  else if (level < threshold_warn)
    return COLOUR_NOTIFY;
  else 
    return COLOUR_WARN;
}

/* Release the hook on the message queue, if we're suddenly killed */
VOID APIENTRY ExeTrap(void)
{
#ifdef USE_DLL
  PMPauseKill();
  #ifdef DEBUG
    fprintf(stderr, "Released hook on message queue.\n");  fflush(stderr);
  #endif
#endif
#ifdef DEBUG
  fprintf(stderr, "All done in exe trap.\n");   fflush(stderr);
#endif  
  DosExitList(EXLST_EXIT, (PFNEXITLIST)ExeTrap);
}

/* Display usage information
 * Two beeps are output before displaying the information since this is a
 * PM program, and tricks are required to capture the stdout error (eg,
 * program | less); the beeps alert to this information being available.
 * (A later version will pop up a window with the information in it.)
 */
void usage(void)
{
  DosBeep(750, 75);  DosBeep(0, 75);  DosBeep(750, 75);
  
  printf("PMPAUSE $Revision: 0.16 $ -- typing/mouse usage monitor\n");
  printf("Copyright Ewen McNeill, 1997.  All rights reserved.\n\n");

  printf("PMPAUSE comes with ABSOLUTELY NO WARRANTY; it is free software\n");
  printf("and you are welcome to redistribute it under certain conditions.\n");
  printf("For further information see the GNU Copyright Licence, version 2.\n\n");
  
  printf("Runtime options (all optional):\n");
  printf("  -h      meter grows horizontally   \\___ mutually exclusive\n");
  printf("  -v      meter grows vertically     /    (default vertical)\n");
  printf("  -m <n>  maximum meter value (default 180)\n");
  printf("  -n <n>  notify threshold (change to notify colour) (default 90)\n");
  printf("  -w <n>  warn threshold (change to warn colour) (default 150)\n");
  printf("  -u <n>  go UP this many units if busy during period (default 4)\n");
  printf("  -d <n>  go DOWN this many units if idle during period (default 12)\n");
  printf("  -c <s>  check every <s> seconds to see if busy/idle (default 4)\n\n");
  printf("  -b      beep when reaching top level, as an alert.\n");
  
  printf("Note: 0 < notify threshold < warn threshold < maximum meter value\n\n");

  printf("  -?      output this help and exit (catch by piping to less, more, etc)\n\n");

  printf("Size/Position is specified with a standard X Windowing System command line\n");
  printf("argument, of the form:\n");
  printf("  <width>x<height>{+-}<xoffset>{+-}yoffset\n");
  printf("Positive offsets relative to bottom, left, negative top, right (OS/2 style)\n");
  printf("  -x       positive relative to top, left negative to bottom, right (like X)\n");
}
