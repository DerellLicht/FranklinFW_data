# makefile for franklin app
# SHELL=cmd.exe
USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = YES
USE_CLANG = NO
# use -static for clang/llvm and cygwin/mingw
USE_STATIC = NO

include ..\tool_select.mak 

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
# ifeq ($(USE_64BIT),YES)
# CFLAGS += -DUSE_64BIT
# endif

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

#**************************************************************************
%.o: %.cpp
	$(TOOLS)/$(GNAME) $(CFLAGS) $< -o $@

BIN = franklin.exe

all: $(BIN)

clean:
	rm -f $(OBJS) *.exe *~ *.zip

dist:
	rm -f franklin.zip
	zip franklin.zip $(BIN) Readme.md LICENSE.txt

wc:
	wc -l $(CPPSRC)

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --check-level=exhaustive --enable=all --std=c++14 --suppressions-list=./.suppress.cppcheck"

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
