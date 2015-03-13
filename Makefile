CFLAGS = -std=c99 -Wall -Wextra -g3 -O3
LDLIBS = -lm

sources := main.c display.c map.c game.c rand.c device_unix.c

gcom : doc/story.o doc/help.o $(addprefix src/,$(sources))
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) persist.gcom gcom gcom.exe doc/*.o

%.o : %.txt
	$(LD) -r -b binary -o $@ $<
