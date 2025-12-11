APP := cs1750_project
SRC := main.cpp Math.cpp GLHelpers.cpp Mesh.cpp Waves.cpp Stone.cpp Input.cpp Boat.cpp Fish.cpp Rod.cpp
OBJ := $(SRC:.cpp=.o)

OS := $(shell uname -s)
HOMEBREW_PREFIX ?= /opt/homebrew

CXX ?= clang++
CPPFLAGS += -std=c++17 -Wall -Wextra -I. -I$(HOMEBREW_PREFIX)/include
LDFLAGS += -L$(HOMEBREW_PREFIX)/lib

ifeq ($(OS), Linux)
  LIBS += -lGL -lGLEW -lglfw
endif

ifeq ($(OS), Darwin)
  CPPFLAGS += -D__MAC__
  LDFLAGS += -framework OpenGL -framework IOKit -framework Cocoa -framework CoreVideo
  LIBS += -lGLEW -lglfw
endif

all: $(APP)

$(APP): $(OBJ)
	$(LINK.cpp) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f $(APP) $(OBJ)
