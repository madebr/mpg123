#ifndef MPG123_PARSE_H
#define MPG123_PARSE_H

#include "frame.h"

int read_frame_init(mpg123_handle* fr);
int frame_bitrate(mpg123_handle *fr);
long frame_freq(mpg123_handle *fr);
int read_frame_recover(mpg123_handle* fr); /* dead? */
int read_frame(mpg123_handle *fr);
void set_pointer(mpg123_handle *fr, long backstep);
int position_info(mpg123_handle* fr, unsigned long no, long buffsize, unsigned long* frames_left, double* current_seconds, double* seconds_left);
double compute_bpf(mpg123_handle *fr);
long time_to_frame(mpg123_handle *fr, double seconds);
int get_songlen(mpg123_handle *fr,int no);
off_t samples_to_bytes(mpg123_handle *fr , off_t s);
off_t bytes_to_samples(mpg123_handle *fr , off_t b);

#endif
