/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp

	mpg123 defines 
	used source: musicout.h from mpegaudio package
*/

#include        <stdio.h>
#include        <string.h>
#include        <signal.h>

#ifndef WIN32
#include        <sys/signal.h>
#include        <unistd.h>
#endif
/* want to suport large files in future */
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifndef off_t
	#define off_t long
#endif
#include        <math.h>



#ifndef _AUDIO_H_
#define _AUDIO_H_

typedef unsigned char byte;

#ifdef OS2
#include <float.h>
#endif

#define MPG123_REMOTE
#define REMOTE_BUFFER_SIZE 2048
#ifdef HPUX
#define random rand
#define srandom srand
#endif

#define SKIP_JUNK 1

#ifdef _WIN32	/* Win32 Additions By Tony Million */
# undef WIN32
# define WIN32

# define M_PI       3.14159265358979323846
# define M_SQRT2	1.41421356237309504880
# define REAL_IS_FLOAT
# define NEW_DCT9

# define random rand
# define srandom srand

# undef MPG123_REMOTE           /* Get rid of this stuff for Win32 */
#endif

#include "xfermem.h"

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

#ifdef REAL_IS_FLOAT
#  define real float
#  define REAL_SCANF "%f"
#  define REAL_PRINTF "%f"
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#  define REAL_SCANF "%Lf"
#  define REAL_PRINTF "%Lf"
#elif defined(REAL_IS_FIXED)
# define real long

# define REAL_RADIX            15
# define REAL_FACTOR           (32.0 * 1024.0)

# define REAL_PLUS_32767       ( 32767 << REAL_RADIX )
# define REAL_MINUS_32768      ( -32768 << REAL_RADIX )

# define DOUBLE_TO_REAL(x)     ((int)((x) * REAL_FACTOR))
# define REAL_TO_SHORT(x)      ((x) >> REAL_RADIX)
# define REAL_MUL(x, y)                (((long long)(x) * (long long)(y)) >> REAL_RADIX)
#  define REAL_SCANF "%ld"
#  define REAL_PRINTF "%ld"

#else
#  define real double
#  define REAL_SCANF "%lf"
#  define REAL_PRINTF "%f"
#endif

#ifndef DOUBLE_TO_REAL
# define DOUBLE_TO_REAL(x)     (x)
#endif
#ifndef REAL_TO_SHORT
# define REAL_TO_SHORT(x)      (x)
#endif
#ifndef REAL_PLUS_32767
# define REAL_PLUS_32767       32767.0
#endif
#ifndef REAL_MINUS_32768
# define REAL_MINUS_32768      -32768.0
#endif
#ifndef REAL_MUL
# define REAL_MUL(x, y)                ((x) * (y))
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#include "audio.h"

/* AUDIOBUFSIZE = n*64 with n=1,2,3 ...  */
#define		AUDIOBUFSIZE		16384

#define         FALSE                   0
#define         TRUE                    1

#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* I suspect that 32767 would be a better idea here, but Michael put this in... */
#define MAXOUTBURST 32768

/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)


struct al_table 
{
  short bits;
  short d;
};

struct frame {
    struct al_table *alloc;
    int (*synth)(real *,int,unsigned char *,int *);
    int (*synth_mono)(real *,unsigned char *,int *);
#ifdef USE_3DNOW
    void (*dct36)(real *,real *,real *,real *,real *);
#endif
    int stereo; /* I _think_ 1 for mono and 2 for stereo */
    int jsbound;
    int single;
    int II_sblimit;
    int down_sample_sblimit;
    int lsf; /* 0: MPEG 1.0; 1: MPEG 2.0/2.5 -- both used as bool and array index! */
    int mpeg25;
    int down_sample;
    int header_change;
    int lay;
    int (*do_layer)(struct frame *fr,int,struct audio_info_struct *);
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int framesize; /* computed framesize */
    int vbr; /* 1 if variable bitrate was detected */
		unsigned long num; /* the nth frame in some stream... */
};

struct parameter {
  int aggressive; /* renice to max. priority */
  int shuffle;	/* shuffle/random play */
  int remote;	/* remote operation */
  int remote_err;	/* remote operation to stderr */
  int outmode;	/* where to out the decoded sampels */
  int quiet;	/* shut up! */
  int xterm_title;	/* Change xterm title to song names? */
  long usebuffer;	/* second level buffer size */
  int tryresync;  /* resync stream after error */
  int verbose;    /* verbose level */
#ifdef HAVE_TERMIOS
  int term_ctrl;
#endif
  int force_mono;
  int force_stereo;
  int force_8bit;
  long force_rate;
  int down_sample;
  int checkrange;
  long doublespeed;
  long halfspeed;
  int force_reopen;
#ifdef USE_3DNOW
  int stat_3dnow; /* automatic/force/force-off 3DNow! optimized code */
  int test_3dnow;
#endif
  long realtime;
  char filename[256];
#ifdef GAPLESS	
  int gapless; /* (try to) remove silence padding/delay to enable gapless playback */
#endif
  long listentry; /* possibility to choose playback of one entry in playlist (0: off, > 0 : select, < 0; just show list*/
  int rva; /* (which) rva to do: 0: nothing, 1: radio/mix/track 2: album/audiophile */
  char* listname; /* name of playlist */
  int long_id3;
};

/* start to use off_t to properly do LFS in future ... used to be long */
struct reader {
  int  (*init)(struct reader *);
  void (*close)(struct reader *);
  int  (*head_read)(struct reader *,unsigned long *newhead);
  int  (*head_shift)(struct reader *,unsigned long *head);
  off_t  (*skip_bytes)(struct reader *,off_t len);
  int  (*read_frame_body)(struct reader *,unsigned char *,int size);
  int  (*back_bytes)(struct reader *,off_t bytes);
  int  (*back_frame)(struct reader *,struct frame *,long num);
  off_t (*tell)(struct reader *);
  void (*rewind)(struct reader *);
  off_t filelen;
  off_t filepos;
  int  filept;
  int  flags;
  unsigned char id3buf[128];
};
#define READER_FD_OPENED 0x1
#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4

extern struct reader *rd,readers[];
extern char *equalfile;
/*ThOr: No fiddling with the pointer in control_generic! */
extern int have_eq_settings;

extern int halfspeed;
extern int buffer_fd[2];
extern txfermem *buffermem;

#ifndef NOXFERMEM
extern void buffer_loop(struct audio_info_struct *ai,sigset_t *oldsigset);
#endif

/* ------ Declarations from "httpget.c" ------ */

extern char *proxyurl;
extern unsigned long proxyip;
/* takes url and content type string address, opens resource, returns fd for data, allocates and sets content type */
extern int http_open (char* url, char** content_type);
extern char *httpauth;

/* ------ Declarations from "common.c" ------ */

extern void audio_flush(int, struct audio_info_struct *);
extern void (*catchsignal(int signum, void(*handler)()))();

extern void print_header(struct frame *);
extern void print_header_compact(struct frame *);
extern void print_id3_tag(unsigned char *buf);

extern int split_dir_file(const char *path, char **dname, char **fname);

extern unsigned int   get1bit(void);
extern unsigned int   getbits(int);
extern unsigned int   getbits_fast(int);
extern void           backbits(int);
extern int            getbitoffset(void);
extern int            getbyte(void);

extern void set_pointer(long);

extern unsigned char *pcm_sample;
extern int pcm_point;
extern int audiobufsize;

extern int OutputDescriptor;

#ifdef VARMODESUPPORT
extern int varmode;
extern int playlimit;
#endif

struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      real *full_gain[3];
      real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern int open_stream(char *,int fd);
extern void read_frame_init (struct frame* fr);
extern int read_frame(struct frame *fr);
/* why extern? */
void prepare_audioinfo(struct frame *fr, struct audio_info_struct *nai);
extern int play_frame(int init,struct frame *fr);
extern int do_layer3(struct frame *fr,int,struct audio_info_struct *);
extern int do_layer2(struct frame *fr,int,struct audio_info_struct *);
extern int do_layer1(struct frame *fr,int,struct audio_info_struct *);
extern void do_equalizer(real *bandPtr,int channel);

#ifdef PENTIUM_OPT
extern int synth_1to1_pent (real *,int,unsigned char *);
#endif
extern int synth_1to1 (real *,int,unsigned char *,int *);
extern int synth_1to1_8bit (real *,int,unsigned char *,int *);
extern int synth_1to1_mono (real *,unsigned char *,int *);
extern int synth_1to1_mono2stereo (real *,unsigned char *,int *);
extern int synth_1to1_8bit_mono (real *,unsigned char *,int *);
extern int synth_1to1_8bit_mono2stereo (real *,unsigned char *,int *);

extern int synth_2to1 (real *,int,unsigned char *,int *);
extern int synth_2to1_8bit (real *,int,unsigned char *,int *);
extern int synth_2to1_mono (real *,unsigned char *,int *);
extern int synth_2to1_mono2stereo (real *,unsigned char *,int *);
extern int synth_2to1_8bit_mono (real *,unsigned char *,int *);
extern int synth_2to1_8bit_mono2stereo (real *,unsigned char *,int *);

extern int synth_4to1 (real *,int,unsigned char *,int *);
extern int synth_4to1_8bit (real *,int,unsigned char *,int *);
extern int synth_4to1_mono (real *,unsigned char *,int *);
extern int synth_4to1_mono2stereo (real *,unsigned char *,int *);
extern int synth_4to1_8bit_mono (real *,unsigned char *,int *);
extern int synth_4to1_8bit_mono2stereo (real *,unsigned char *,int *);

extern int synth_ntom (real *,int,unsigned char *,int *);
extern int synth_ntom_8bit (real *,int,unsigned char *,int *);
extern int synth_ntom_mono (real *,unsigned char *,int *);
extern int synth_ntom_mono2stereo (real *,unsigned char *,int *);
extern int synth_ntom_8bit_mono (real *,unsigned char *,int *);
extern int synth_ntom_8bit_mono2stereo (real *,unsigned char *,int *);

extern void rewindNbits(int bits);
extern int  hsstell(void);
extern void set_pointer(long);
extern void huffman_decoder(int ,int *);
extern void huffman_count1(int,int *);
extern void print_stat(struct frame *fr,unsigned long no,long buffsize,struct audio_info_struct *ai);
extern int get_songlen(struct frame *fr,int no);

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables(long scale);
extern int make_conv16to8_table(int);
extern void dct64(real *,real *,real *);

#ifdef USE_MMX
extern void dct64_MMX(short *a,short *b,real *c);
extern int synth_1to1_MMX(real *, int, short *, short *, int *);
#endif

extern int synth_ntom_set_step(long,long);

extern int control_generic(struct frame *fr);

extern int cdr_open(struct audio_info_struct *ai, char *ame);
extern int au_open(struct audio_info_struct *ai, char *name);
extern int wav_open(struct audio_info_struct *ai, char *wavfilename);
extern int wav_write(unsigned char *buf,int len);
extern int cdr_close(void);
extern int au_close(void);
extern int wav_close(void);

extern int au_open(struct audio_info_struct *ai, char *aufilename);
extern int au_close(void);

extern int cdr_open(struct audio_info_struct *ai, char *cdrfilename);
extern int cdr_close(void);

extern unsigned char *conv16to8;
extern long freqs[9];
extern real muls[27][64];
extern real decwin[512+32];
#ifndef USE_MMX
extern real *pnts[5];
#endif

extern real equalizer[2][32];
extern real equalizer_sum[2][32];
extern int equalizer_cnt;

extern struct audio_name audio_val2name[];

extern struct parameter param;

/* 486 optimizations */
#define FIR_BUFFER_SIZE  128
extern void dct64_486(int *a,int *b,real *c);
extern int synth_1to1_486(real *bandPtr,int channel,unsigned char *out,int nb_blocks);

/* 3DNow! optimizations */
#ifdef USE_3DNOW
extern int getcpuflags(void);
extern void dct36(real *,real *,real *,real *,real *);
extern void dct36_3dnow(real *,real *,real *,real *,real *);
extern int synth_1to1_3dnow(real *,int,unsigned char *,int *);
#endif

/* avoid the SIGINT in terminal control */
void next_track(void);
extern long outscale;

#endif
