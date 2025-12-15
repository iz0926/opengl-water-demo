APP := cs1750_project
SRC := main.cpp Math.cpp GLHelpers.cpp Mesh.cpp Waves.cpp Stone.cpp Input.cpp Boat.cpp Fish.cpp Rod.cpp Chest.cpp Audio.cpp \
       imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
       imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp
OBJ := $(SRC:.cpp=.o)

OS := $(shell uname -s)
HOMEBREW_PREFIX ?= /opt/homebrew

CXX ?= clang++
CPPFLAGS += -std=c++17 -Wall -Wextra -I. -I$(HOMEBREW_PREFIX)/include -I$(HOMEBREW_PREFIX)/include/SDL2 -Iimgui -Iimgui/backends -DIMGUI_IMPL_OPENGL_LOADER_GLEW
LDFLAGS += -L$(HOMEBREW_PREFIX)/lib
LIBS += -lSDL2 -lSDL2_mixer

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
