/*
	common: anything can happen here... frame reading, output, messages

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#ifndef _MPG123_COMMON_H_
#define _MPG123_COMMON_H_

/* max = 1728 */
#define MAXFRAMESIZE 3456
/*
	AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
	A: sync
	B: mpeg version
	C: layer
	D: CRC
	E: bitrate
	F:sampling rate
	G: padding
	H: private
	I: channel mode
	J: mode ext
	K: copyright
	L: original
	M: emphasis

	old compare mask 0xfffffd00:
	11111111 11111111 11111101 00000000

	means: everything must match excluding padding and channel mode, ext mode, ...
	But a vbr stream's headers will differ in bitrate!
	We are already strict in allowing only frames of same type in stream, we should at least watch out for VBR while being strict.

	So a better mask is:
	11111111 11111111 00001101 00000000

	(still unsure about this private bit)
*/
#define HDRCMPMASK 0xffff0d00

extern unsigned long firsthead;
extern int tabsel_123[2][3][16];
extern double compute_tpf(struct frame *fr);
extern double compute_bpf(struct frame *fr);
extern long compute_buffer_offset(struct frame *fr);

struct bitstream_info {
  int bitindex;
  unsigned char *wordpointer;
};

extern struct bitstream_info bsi;

/* well, I take that one for granted... at least layer3 */
#define DECODER_DELAY 529

#ifdef GAPLESS
unsigned long samples_to_bytes(unsigned long s, struct frame *fr , struct audio_info_struct* ai);
/* samples per frame ...
Layer I
Layer II
Layer III
MPEG-1
384
1152
1152
MPEG-2 LSF
384
1152
576
MPEG 2.5
384
1152
576
*/
#define spf(fr) (fr->lay == 1 ? 384 : (fr->lay==2 ? 1152 : (fr->lsf || fr->mpeg25 ? 576 : 1152)))
/* still fine-tuning the "real music" window... see read_frame */
#define GAP_SHIFT -1
#endif

/* for control_generic */
extern const char* remote_header_help;
void make_remote_header(struct frame* fr, char *target);

int position_info(struct frame* fr, unsigned long no, long buffsize, struct audio_info_struct* ai,
                   unsigned long* frames_left, double* current_seconds, double* seconds_left);

int read_frame_recover(struct frame* fr);

off_t frame_index_find(unsigned long want_frame, unsigned long* get_frame);
void print_frame_index(FILE* out);

/* this could become a struct... */
extern long lastscale;
extern int rva_level[2];
extern float rva_gain[2];
extern float rva_peak[2];

#endif
