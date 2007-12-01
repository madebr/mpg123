targets = mpg123_to_wav scan id3dump mpglib

all: $(targets)

MPG123_CFLAGS  := $(shell pkg-config --cflags libmpg123)
MPG123_LDFLAGS := $(shell pkg-config --libs   libmpg123)
SND_CFLAGS     := $(shell pkg-config --cflags sndfile)
SND_LDFLAGS    := $(shell pkg-config --libs   sndfile)

# Oder of libs not that important here...
compile = $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(MPG123_CFLAGS) $(MPG123_LDFLAGS)

mpg123_to_wav: mpg123_to_wav.c
	$(compile) $(SND_CFLAGS) $(SND_LDFLAGS)-o mpg123_to_wav mpg123_to_wav.c

scan: scan.c
	$(compile) -o scan scan.c

id3dump:id3dump.c
	$(compile) -o id3dump id3dump.c

mpglib: mpglib.c
	$(compile) -o mpglib mpglib.c

clean:
	rm -vf $(targets)
