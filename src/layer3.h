void layer3_gapless_init(unsigned long b, unsigned long e);
/* should I wonder about playback position != decoder position? */
/* decrease position */
void layer3_gapless_rewind(unsigned long frames, struct frame* fr, struct audio_info_struct *ai);
/* increase position */
void layer3_gapless_forward(unsigned long frames, struct frame* fr, struct audio_info_struct *ai);
void layer3_gapless_set_position(unsigned long frames, struct frame* fr, struct audio_info_struct *ai);
