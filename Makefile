CFLAGS = -std=c99 -Wall -Wextra -g3 -Os

gcom : main.c display.c device_unix.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) gcom gcom.exe *.o
