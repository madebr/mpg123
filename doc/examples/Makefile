
CFLAGS := -g -Wall $(shell pkg-config --cflags libmpg123 sndfile)
LDFLAGS := $(shell pkg-config --libs libmpg123 sndfile)

mpg123_to_wav: mpg123_to_wav.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o mpg123_to_wav mpg123_to_wav.c

clean:
	rm -f mpg123_to_wav
