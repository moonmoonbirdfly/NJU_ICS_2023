REPO_PATH = repo
ifeq ($(wildcard repo/include/BirdGame.h),)
  $(shell git clone --depth=1 git@github.com:NJU-ProjectN/sdlbird.git $(REPO_PATH))
endif

IFLAGS:= -I/home/quantum6/lib-build/include/SDL2 -I. -I$(SRC_LINK)/
NAME = bird
SRCS = $(shell find $(REPO_PATH)/src/ -name "*.cpp")
INC_PATH = $(REPO_PATH)/include
LIBS += libminiSDL libSDL_image libfixedptc libbmp libbdf
include $(NAVY_HOME)/Makefile

install: install-file
init:;

install-file:
	ln -sf -T $(abspath $(REPO_PATH)/res) $(NAVY_HOME)/fsimg/share/games/bird

.PHONY: install-file init
