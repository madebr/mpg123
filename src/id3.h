
#ifndef MPG123_ID3_H
#define MPG123_ID3_H

void init_id3();
void exit_id3();
void reset_id3();
void print_id3_tag(unsigned char *id3v1buf);
int parse_new_id3(unsigned long first4bytes, struct reader *rds);

#endif
