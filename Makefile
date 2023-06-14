PROJECT = mbest2mkv


CFILES = mbestformat.c util.c
CXXFILES = mbest2mkv.cc

ifeq ($(OS), Windows_NT)
RM = del
TARGET = $(PROJECT).exe
else
RM = rm
TARGET = $(PROJECT)
endif

INCLUDES=.
DEFINES=

CC = gcc
CXX = g++
LD = g++

PREFIX ?= $(HOME)/.local

COMMONFLAGS = $(patsubst %, -I%, $(INCLUDES)) $(patsubst %, -D%, $(DEFINES))

_CFLAGS = $(CFLAGS) $(COMMONFLAGS)
_CXXFLAGS = $(CXXFLAGS) $(COMMONFLAGS)

_LDFLAGS = $(LDFLAGS) -lmatroska -lebml

OBJS = $(patsubst %.cc, %.o, $(CXXFILES)) $(patsubst %.c, %.o, $(CFILES))


all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $^ -o $@ $(_LDFLAGS)

%.o: %.cc
	$(CXX) $(_CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(_CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean install
