/* PMPAUSEDLL -- monitors keyboard/mouse usage
 *
 * This module is part of PM Pause, see pmpause.c for details.
 *
 * Written by Ewen McNeill <ewen@naos.co.nz>, Dec 1997.
 * Copyright Ewen McNeill, 1997.
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
 * The structure is based loosely on xraydll (from EDM vol 5, issue 1),
 * see http://www.edm2.com/ for more details.
 *
 * WARNING: there _is_ a race condition here with the update/clear of the
 * count, in that it may be updated by different threads in parallel, and
 * the exact result of doing so may be undefined.  However, the instructions
 * used should, where possible, compile into a single machine code instruction
 * and thus on a single processor be atomic. Where this doesn't happen, the
 * overlapped instructions could result in the wrong count, but if the count
 * is too high that is not a big problem (only want to tell something from
 * nothing), and if it is too low, it'll be bumped up by the next action.
 *
 * We don't care too much about the margins, so it's faster to leave the
 * race condition than bother with locks around it.  If necessary some
 * locking could be added.
 *---------------------------------------------------------------------------
 * $Id: pmpdll.c 0.6 1998/01/13 02:18:32 ewen Exp $
 */

/* OS/2 Header portions required */
#define INCL_WIN
#define INCL_DOSMODULEMGR
#include <os2.h>

#include "pmpause.h"

static BOOL Hooked    = FALSE;      /* flag set when hooks are enabled      */
static HMODULE hmDll  = NULLHANDLE; /* Handle to our own module (DLL)       */
static HAB  habClient = NULLHANDLE; /* Handle to client anchor block        */
static unsigned long keymouse = 0;  /* Number of keyboard/mouse messsages   */
                                    /* since last cleared                   */

/*#define TESTHOOK*/                /* If TESTHOOK is defined we hook only  */
                                    /* our process; otherwise system wide   */
#ifdef TESTHOOK
  HMQ hmqType = HMQ_CURRENT;        /* HMQ_CURRENT ==> current thread only  */
#else
  HMQ hmqType = NULLHANDLE;         /* NULLHANDLE  ==> system wide          */
#endif

#define JUST_CLICKS                 /* If defined, only count mouse button  */
                                    /* clicks (up/down), not movements      */

/* Hook for Input queue (for posted messages).
 *
 * We simply count the number of WM_CHAR, and WM_MOUSEMOVE messages that we
 * see.  There will be at least two WM_CHAR messages for each keypress
 * (up/down), and several WM_MOUSEMOVE messages for each mouse action, but
 * we don't care -- we are only trying to distinguish "something" from
 * "nothing".
 *
 * We always return FALSE (ie, continue processing) since we don't want to
 * intercept the message, just note its passing.
 */
BOOL EXPENTRY PMPauseInputHook(HAB hab, PQMSG pQmsg, USHORT fs)
{
  switch(pQmsg->msg)
  {
#ifdef JUST_CLICKS
    case WM_BUTTON1DOWN:           /* Just count button clicks, rest is idle */
    case WM_BUTTON1UP:
    case WM_BUTTON2DOWN:
    case WM_BUTTON2UP:
    case WM_BUTTON3DOWN:
    case WM_BUTTON3UP:
#else    
    case WM_MOUSEMOVE:             /* Count all mouse movement; note this    */
#endif                             /* will catch program simulated movement  */
    case WM_CHAR:
      keymouse++;                  /* Mouse and keyboard -- count 'em        */
      break;

    default:                       /* Ignore the rest, we don't need 'em     */
      break;
  }

  return(FALSE);                   /* Keep processing the messages           */
}

/* Return count of key/mouse actions since last cleared                      */
unsigned long EXPENTRY EXPORT PMPauseGetCount(void)
{
  return keymouse;
}

/* Clear count of key/mouse actions                                          */
void EXPENTRY EXPORT PMPauseClearCount(void)
{
  keymouse = 0;
}

/* Initialisation -- hook the queue that we want.                            */
BOOL EXPENTRY EXPORT PMPauseInit(HWND window)
{
  if(! Hooked)
  {
    if(DosQueryModuleHandle(DLLNAME, &hmDll))
      return FALSE;
      
    habClient = WinQueryAnchorBlock(window);
    
    WinSetHook(habClient, hmqType, HK_INPUT, (PFN)PMPauseInputHook, hmDll);

    Hooked = TRUE;
  }
  
  return TRUE;
}

/* Disable hooks again                                                      */
BOOL EXPENTRY EXPORT PMPauseKill(void)
{
  if(Hooked)
  {
    WinReleaseHook(habClient, hmqType, HK_INPUT, (PFN)PMPauseInputHook, hmDll);

    Hooked = FALSE;
  }

  return TRUE;	    
}
