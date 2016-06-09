TAG  := ringbuffer

LIB  := lib$(TAG).a
TEXE := $(TAG)-test

ifeq ($(shell uname),Darwin)
CC  := clang
CXX := clang++ -stdlib=libc++
LD  := $(CXX)
else
CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LD  := $(CROSS_COMPILE)g++
endif

LD_LIBRARY_PATH := /usr/local/lib

CFLAGS   :=
CFLAGS   += -g -O0 -Wall
CFLAGS   += -I/usr/local/include/gtest
ifeq ($(shell uname),Darwin)
CFLAGS  += -Wl,-macosx_version_min,10.5
endif

CXXFLAGS := $(CFLAGS)

LDFLAGS  := -g -O0
LDFLAGS  += -L.
LDFLAGS  += -L$(LD_LIBRARY_PATH)

CSRC   := $(shell find * -name '*.c')
CPPSRC := $(shell find * -name '*.cpp' -o -name '*.cxx')
HSRC   := $(shell find * -name '*.h')

COBJ := $(CSRC:.c=.o)
CPPOBJ := $(patsubst %.cpp,%.o,$(patsubst %.cxx,%.o,$(CPPSRC)))

OBJ := $(COBJ) $(CPPOBJ)

.PHONY: all clean check

all: $(TEXE)

%.o: %.c $(HSRC) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cxx $(HSRC) Makefile
	$(CC) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp $(HSRC) Makefile
	$(CC) $(CXXFLAGS) -c $< -o $@

$(LIB): $(OBJ)
ifeq ($(shell uname),Darwin)
	libtool -static -o $@ $^
else
	$(AR) rv $(LIB) $^
endif

$(TEXE): $(LIB)
	$(LD) $(LDFLAGS) -o $@ $(TEXE).o -l$(TAG) -lgtest -lgtest_main

check: $(TEXE)
ifeq ($(shell uname),Darwin)
	DYLD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./$(TEXE)
else
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./$(TEXE)
endif

clean:
	rm -f $(TEXE) $(OBJ) $(LIB) *.o *.a