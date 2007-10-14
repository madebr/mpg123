#ifndef MPG123_ID3_H
#define MPG123_ID3_H

/* really need it _here_! */
#include "frame.h"

void init_id3(mpg123_handle *fr);
void exit_id3(mpg123_handle *fr);
void reset_id3(mpg123_handle *fr);
int  parse_new_id3(mpg123_handle *fr, unsigned long first4bytes);

#endif
