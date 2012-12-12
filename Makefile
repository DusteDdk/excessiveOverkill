CC = gcc
LD = $(CC)
STRIP = strip

NAME=ExcessiveOverkill.so
TARGET= $(NAME)

INCS =  -I. -I./3rd -I/usr/include -I/usr/include/SDL
CFLAGS = -shared -fPIC
LDFLAGS=$(CFLAGS) 
LIBS = -lSDL -lSDL_image -lSDL_mixer -lSDL_ttf -lpng -lm -lz -lpthread -lGL -lGLU

OBJS = camera.o console.o data.o eng.o game.o gfxeng.o gltxt.o\
gui.o gui-test.o input.o list.o particles.o screenshot.o sound.o\
sprite.o strings.o tick.o vboload.o 3rd/glew.o

########################################################################

release: CFLAGS += -O2 -DEO_RELEASE
release: $(TARGET)
		$(STRIP) $(TARGET)

debug: CFLAGS += -Wall -O0 -g -DEO_DEBUG
debug: $(TARGET)

$(TARGET): $(OBJS)
		$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

.c.o:
		$(CC) $(CFLAGS) $(INCS) $(DEFS) -c $< -o $@

clean:
		rm -f *.o $(NAME) 3rd/*.o
