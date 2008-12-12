ifeq ($(CC),)
CC=gcc
endif

bins=rmsdouble.bin rms16.bin s16_double.bin f32_double.bin hex24_double.bin

.PHONY: all clean

all: $(bins)

clean:
	rm -vf $(bins)

%.bin: %.c
	$(CC) -O -o $@ $< -lm

# That generic target does not work. Ignored. Why?
.c.bin:
	$(CC) -O -o $@ $< -lm

%.double: %.hex hex24_double.bin
	./hex24_double.bin < $< > $@

%.double: %.s16 s16_double.bin
	./s16_double.bin < $< > $@

%.double: %.f32 f32_double.bin
	./f32_double.bin < $< > $@

%.double: %.f64
	cp $< $@
