CFLAGS = -std=c99 -Wall -Wextra -g3 -Os
LDLIBS = -lm

gcom : main.c display.c map.c game.c device_unix.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) gcom gcom.exe *.o
