/* init part 1; set start/end in samples_*/
void layer3_gapless_init(unsigned long b, unsigned long e);
/* init part 2; transform to byte addresses with new info */
void layer3_gapless_bytify(struct frame *fr, struct audio_info_struct *ai);
/* after some seeking action to a new frame, the decoder needs to know which one is coming next */
void layer3_gapless_set_position(unsigned long frames, struct frame* fr, struct audio_info_struct *ai);
/* removing the gaps from buffer */
void layer3_gapless_buffercheck();
