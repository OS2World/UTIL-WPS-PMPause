# EMX/GCC/GMAKE makefile for PMPause
#
# Developed by Ewen McNeill, Dec 1997.
# Copyright Ewen McNeill, 1997.
#
CFLAGS=-s -Wall -Zomf -Zmtd -O2 #-DDEBUG
DLLCFLAGS=-s -Wall -Zdll -Zno-rte -Zomf -mprobe -O2 #-DDEBUG
CC=gcc
EMXIMP=emximp
LFLAGS=

DLLNAME=pmpdll
PROGNAME=pmpause
PROGINC=$(PROGNAME).h

.SUFFIXES:

.SUFFIXES: .c .rc

all: $(PROGNAME).exe $(DLLNAME).dll

$(PROGNAME).exe: $(PROGNAME).c $(PROGINC) $(PROGNAME).def $(DLLNAME).lib
	$(CC) $(CFLAGS) $(LFLAGS) -o $(PROGNAME).exe $(PROGNAME).c $(PROGNAME).def -L. -l$(DLLNAME)

$(DLLNAME).dll: $(DLLNAME).c $(PROGINC) $(DLLNAME).def
	$(CC) $(DLLCFLAGS) -o $(DLLNAME).dll $(DLLNAME).c $(DLLNAME).def

$(DLLNAME).lib: $(DLLNAME).def 
	$(EMXIMP) -o $(DLLNAME).lib $(DLLNAME).def

ALLFILES = $(PROGNAME).exe $(DLLNAME).dll $(PROGNAME).c $(PROGINC) $(DLLNAME).c $(PROGNAME).def $(DLLNAME).def README COPYING Makefile
ZIPNAME  = pmpause.zip

$(ZIPNAME): $(ALLFILES)
	zip $(ZIPNAME) $(ALLFILES)

