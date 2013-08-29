                                PMPAUSE 0.16
                      Written by Ewen McNeill, Dec 1997
                        Copyright Ewen McNeill, 1997.
                              <ewen@naos.co.nz>

WARNING: This is BETA test software.  Because of the way it operates
it must hook into the OS/2 message queue.  Thus undiscovered bugs in
this software may seriously affect the stability of your system.
Please read the information below carefully.


INTRODUCTION

PMPause is a small (7K executable, plus 3K DLL) OS/2 PM based program
designed to remind you that it is time to take a break from typing and
stretch a little.  It does this by monitoring your keyboard usage, and
mouse usage, and displaying a small (or if you wish, large) graph
which rises when you have been using the keyboard/mouse recently, and
falls when you have not been using the keyboard/mouse recently.
Optionally it can make a small beep when the graph reaches the top limit, 
as an additional hint that you should take a break.

This program could form part of your strategy to avoid, or manage, OOS
(RSI), but you SHOULD NOT rely on the program alone.  It is intended
to remind you to take micropause type breaks, but does not force you
to take them.  

For additional information on OOS, and techniques for avoiding or
managing the problem see type Typing Injury FAQ which can be found at:

         http://www.tifaq.com/


COPYRIGHT

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


INSTALLATION

There is no installation program at present.  The program can be run
from the command line, or you can create a suitable object that causes
the program to run and place that in your Startup folder.

The only requirement is that the program be able to find its companion
DLL (pmpdll.dll); either change to the directory where the program and
DLL are present before running the program (or set the objects working
directory to that directory), or copy the DLL into a directory on your
LIBPATH.


USAGE

The program is configurable from the command line (only; well you can
recompile the enclosed source too if you wish).  The following command
line options are supported:

Runtime options (all optional):
  -h      meter grows horizontally   \___ mutually exclusive
  -v      meter grows vertically     /    (default vertical)
  -m <n>  maximum meter value (default 180)
  -n <n>  notify threshold (change to notify colour) (default 90)
  -w <n>  warn threshold (change to warn colour) (default 150)
  -u <n>  go UP this many units if busy during period (default 4)
  -d <n>  go DOWN this many units if idle during period (default 12)
  -c <s>  check every <s> seconds to see if busy/idle (default 4)

  -b      beep when reaching top level, as an alert.
Note: 0 < notify threshold < warn threshold < maximum meter value

  -?      output this help and exit (catch by piping to less, more, etc)

Size/Position is specified with a standard X Windowing System command line
argument, of the form:
  <width>x<height>{+-}<xoffset>{+-}yoffset
Positive offsets relative to bottom, left, negative top, right (OS/2 style)
  -x       positive relative to top, left negative to bottom, right (like X)

(The above information comes from the program's internal help obtained
by running: pmpause | more.)

There are two separate things which can be configured about the
program.  One is the logical size of the meter, that is its limits and
boundaries, and the other is the physical size that it is displayed
(and where it is displayed).

The logical size is set with the "-m" parameter to set the maximum
value; the meter will then logically run from 0 through to that value.
Within that range are two thresholds, the notify threshold ("-n") and
the warn threshold ("-w").  On crossing each threshold the graph
colour will change to a more "urgent" colour as a visual indication
that the meter level is getting higher.

The next three values interact to determine how long it will take for
the meter to reach the top limit.  The "-c" value determines how often
the program checks to recent keyboard/mouse activity.  Then the "-u"
value is how far up the meter will go if keyboard/mouse activity was
detected in that time; the "-d" value is how far down it will go if
there was NO keyboard/mouse activity in that time.  The "-c" value
should be kept reasonably small, so that the program can notice when
you take small pauses.

If you specify the "-b" flag, when the meter reaches the top limit it
will beep once to alert you to that fact. The beep is a very short
blip to avoid distracting other people in the office (this can be
changed in the source).

The physical display of the meter is controlled by the display string,
which is in the format:

    <width>x<height>{+-}<xoffset>{+-}yoffset

that is that the string:

    10x100+0+0

would display a window 10 wide, by 100 high, at 0 offset from the left
edge, and 0 offset from the bottom edge.  Negative values are
interpreted as being relative to the opposite edge of the screen.

By default this string is NOT interpreted exactly the same way that
programs written for the X Windowing System interpret the string;
instead it is interpreted in OS/2 coordinates (where 0,0 is the lower
left of the screen).  If you wish it to be interpreted as an X program
would, give PMPause the "-x" parameter as well.

This table shows the way that positive and negative values are interpreted:

    Without -x:  positive relative to bottom, left,
                 negative relative to top, right
    With    -x:  positive relative to top, left,
                 negative relative to bottom, right

The PMPause window is displayed without sizing bars because they take
up far too much space for such a small window.  For the same reason,
the window does not have a title bar.  It is recommended that the
window is kept fairly small, and put in one corner of the screen.

Finally the "-v" flag says that the meter should grow vertically
(upwards), and the "-h" flag says that the meter should grow
horizontally (left to right).  These flags are mutually exclusive.  It
is permissible to use the vertical mode with a window that is wider
than it is high, but it is not particularly useful to do so.

To quit the program either make the window active (move the mouse into
the window, and click on it), and then type "q", or close it from the
OS/2 Window List, or with a command line program.


HOW IT WORKS

PMPause comes with a small DLL (pmpdll.dll) which hooks the OS/2
PM message queue looking for keystroke messages, and mouse messages.
(The supplied binary looks only for button click messages because the
OS/2 Lockup uses mouse movement messages while it is working.)

Because it has hooked the system message queue, the DLL is effectively
part of every PM program running on your system, so it can see the
keystrokes and mouse messages sent to all those programs.

The DLL then counts the number of keystrokes and mouse messages since
the value was last reset.  The main program queries this value
periodically and uses what it finds out (some activity versus no
activity) to determine how to move the graph.

The program does not hook, or count, keystrokes sent to full screen
OS/2 sessions.  A future version may be able to do this, but this is
not a priority, as there would be no way to view the graph while a
full screen session was active. 

I do not know if the program is able to count keystrokes to a windows
Win-OS/2 session; I do not use Win-OS/2 myself.  (Reports on this part
appreciated.)


BETA TEST

Because of the way that the program operates (see above), undiscovered
bugs in this program can cause serious system instability.  This
program has been tested on my system (OS/2 Warp 3 Connect, fixpack 26)
for several weeks prior to this BETA release.  I am not aware of any
problems which affect system performance.  However this BETA release
is being made to allow wider testing of the program and uncover any
lurking bugs.

Use of this BETA software is completely at your own risk.  I recommend
against using this software on production systems at this time; a non
BETA version will be released later after some time for testing has
passed, and any bugs noticed fixed. 

If you do encounter problems while running the PMPause program, the
first thing you should do is quit the program (click on its window
then type "q"), and see if the problem persists.  If the problem goes
away, you may wish to try running PMPause again and see if the problem
reoccurs.  If the problem does not go away, you may have to reboot
your system to clear the problem.

I would appreciate reports of any problems, or success, using this
program, particularly from users of OS/2 Warp 4, and people running
other fixpacks from the one I have installed (Warp 3, fixpack 26).
Please send all reports to: ewen@naos.co.nz


REBUILDING THE PROGRAM

This program is supplied with a Makefile (for GNU make), and can be
rebuilt using the gcc compiler supplied with the EMX package by 
Eberhard Mattes.  GNU make and the EMX development environment can be
downloaded from OS/2 FTP sites around the world.


CONCLUSION

The source code has been included with this program.  I would
appreciate copies of any changes that you make.  

I would also appreciate feedback as to whether the program has been
useful for you, and suggestions of extra features.  Please keep in
mind, though, that one of the aims of the program is to be very small
so that it has a minimal impact on even memory constrained systems.


Ewen McNeill <ewen@naos.co.nz>
Jan 1998.
