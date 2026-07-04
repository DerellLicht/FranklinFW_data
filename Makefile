# makefile for franklin app
# SHELL=cmd.exe
USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = YES
USE_CLANG = NO
# use -static for clang and cygwin/mingw
# with -static 340KB, without -static: 33KB
USE_STATIC = NO

#  clang++ vs tdm g++
#  clang gives *much* clearer compiler error messages...
#  However, programs built with clang++ will require libc++.dll and libunwind.dll
#  in order to be used elsewhere 
#  (unless built with -static, which significantly boosts file size)
#  That is why the executable files are smaller than TDM ...
ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
else
ifeq ($(USE_CLANG),YES)
#TOOLS=d:\clang\bin
TOOLS=d:\llvm\bin
else
TOOLS=d:\tdm32\bin
endif
endif

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
ifeq ($(USE_64BIT),YES)
CFLAGS += -DUSE_64BIT
endif

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LiFLAGS += -dUNICODE -d_UNICODE
LFLAGS += -dUNICODE -d_UNICODE
endif

ifeq ($(USE_CLANG),YES)
CFLAGS += -DUSING_CLANG
endif
LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
IFLAGS += -Ider_libs

# This is required for *some* versions of makedepend
IFLAGS += -DNOMAKEDEPEND

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

CPPSRC=franklin.cpp file_handler.cpp \
der_libs/conio_min.cpp \
der_libs/common_funcs.cpp \
der_libs/qualify.cpp 

LINTFILES=lintdefs.cpp lintdefs.ref.h 

OBJS = $(CPPSRC:.cpp=.o)

LIBS=-lshlwapi -lcomdlg32

GNAME=g++
#GNAME=clang++

#**************************************************************************
%.o: %.cpp
	$(TOOLS)/$(GNAME) $(CFLAGS) -c $< -o $@

ifeq ($(USE_64BIT),NO)
BIN = franklin.exe
else
BIN = franklin64.exe
endif

all: $(BIN)

clean:
	rm -f $(OBJS) *.exe *~ *.zip

dist:
	rm -f franklin.zip
	zip franklin.zip $(BIN) Readme.md

wc:
	wc -l $(CPPSRC)

check:
	cmd /C "d:\llvm\bin\clang-tidy.exe $(CPPSRC)"

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) $(LINTFILES) $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC)

$(BIN): $(OBJS)
	$(TOOLS)/$(GNAME) $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 

# DO NOT DELETE

franklin.o: der_libs/common.h der_libs/conio_min.h franklin.h
franklin.o: der_libs/qualify.h
file_handler.o: der_libs/common.h der_libs/conio_min.h franklin.h
der_libs/conio_min.o: der_libs/common.h der_libs/conio_min.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/qualify.o: der_libs/common.h der_libs/qualify.h
