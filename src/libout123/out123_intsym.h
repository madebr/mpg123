#ifndef OUT123_INTSYM_H
#define OUT123_INTSYM_H
/* Mapping of internal mpg123 symbols to something that is less likely to conflict in case of static linking. */
#define catchsignal IOT123_catchsignal
#define safe_realloc IOT123_safe_realloc
#define compat_open IOT123_compat_open
#define compat_fopen IOT123_compat_fopen
#define compat_close IOT123_compat_close
#define compat_fclose IOT123_compat_fclose
#define win32_wide_utf8 IOT123_win32_wide_utf8
#define win32_utf8_wide IOT123_win32_utf8_wide
#define unintr_write IOT123_unintr_write
#define unintr_read IOT123_unintr_read
#define open_module IOT123_open_module
#define close_module IOT123_close_module
#define list_modules IOT123_list_modules
#define buffer_init IOT123_buffer_init
#define buffer_exit IOT123_buffer_exit
#define buffer_sync_param IOT123_buffer_sync_param
#define buffer_open IOT123_buffer_open
#define buffer_encodings IOT123_buffer_encodings
#define buffer_start IOT123_buffer_start
#define buffer_ndrain IOT123_buffer_ndrain
#define buffer_stop IOT123_buffer_stop
#define buffer_close IOT123_buffer_close
#define buffer_continue IOT123_buffer_continue
#define buffer_ignore_lowmem IOT123_buffer_ignore_lowmem
#define buffer_drain IOT123_buffer_drain
#define buffer_end IOT123_buffer_end
#define buffer_pause IOT123_buffer_pause
#define buffer_drop IOT123_buffer_drop
#define buffer_write IOT123_buffer_write
#define buffer_fill IOT123_buffer_fill
#define read_buf IOT123_read_buf
#define xfermem_init IOT123_xfermem_init
#define xfermem_init_writer IOT123_xfermem_init_writer
#define xfermem_init_reader IOT123_xfermem_init_reader
#define xfermem_get_freespace IOT123_xfermem_get_freespace
#define xfermem_get_usedspace IOT123_xfermem_get_usedspace
#define xfermem_getcmd IOT123_xfermem_getcmd
#define xfermem_getcmds IOT123_xfermem_getcmds
#define xfermem_putcmd IOT123_xfermem_putcmd
#define xfermem_writer_block IOT123_xfermem_writer_block
#define xfermem_write IOT123_xfermem_write
#define xfermem_done IOT123_xfermem_done
#define au_open IOT123_au_open
#define cdr_open IOT123_cdr_open
#define raw_open IOT123_raw_open
#define wav_open IOT123_wav_open
#define wav_write IOT123_wav_write
#define wav_close IOT123_wav_close
#define au_close IOT123_au_close
#define raw_close IOT123_raw_close
#define cdr_formats IOT123_cdr_formats
#define au_formats IOT123_au_formats
#define raw_formats IOT123_raw_formats
#define wav_formats IOT123_wav_formats
#define wav_drain IOT123_wav_drain
#define write_parameters IOT123_write_parameters
#define read_parameters IOT123_read_parameters
#ifndef HAVE_STRERROR
#define strerror IOT123_strerror
#endif
#ifndef HAVE_STRDUP
#define strdup IOT123_strdup
#endif
#endif
