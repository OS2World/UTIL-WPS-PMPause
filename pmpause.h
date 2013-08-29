/* PMPAUSE -- header file to define functions, etc.
 *
 * Copyright Ewen McNeill <ewen@naos.co.nz>, 1997. 
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
 * $Id: pmpause.h 0.3 1998/01/13 02:18:59 ewen Exp $
 */

#ifndef PMPAUSE_H
#define PMPAUSE_H $Version:$

#if    defined(__BORLANDC__)
#define EXPORT _export
#elif  defined(__EMX__)
#define EXPORT
#else
#define EXPORT _Export
#endif

#define DLLNAME "pmpdll.dll"

unsigned long EXPENTRY PMPauseGetCount(void);
void          EXPENTRY PMPauseClearCount(void);
BOOL          EXPENTRY PMPauseInit(HWND window);
BOOL          EXPENTRY PMPauseKill(void);

#endif
