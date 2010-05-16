/*
	lfs_alias: Aliases to the small/native API functions with the size of long int as suffix.

	copyright 2010 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	Use case: Client code on Linux/x86-64 that defines _FILE_OFFSET_BITS to 64, which is the only choice on that platform anyway. It should be no-op, but prompts the platform-agnostic header of mpg123 to define API calls with the corresponding suffix.
	This file provides the names for this case. It's cruft, but glibc does it, too -- so people rely on it.
	Oh, and it also caters for the lunatics that define _FILE_OFFSET_BITS=32 on 32 bit platforms.
*/

#include "config.h"

#ifndef LFS_ALIAS_BITS
#error "I need the count of alias bits here."
#endif

/* Use the plain function names. */
#define MPG123_NO_LARGENAME MPG123_MACROCAT(_, LFS_ALIAS_BITS)
#include "mpg123.h"

/* Now get the rest of the infrastructure on speed, namely attribute_align_arg, to stay safe. */
#include "mpg123lib_intern.h"

#define MACROCAT_REALLY(a, b) a ## b
#define MACROCAT(a, b) MACROCAT_REALLY(a, b)
#define ALIAS_SUFFIX MACROCAT(_, LFS_ALIAS_BITS)
#define ALIAS_NAME(func) MACROCAT(func, ALIAS_SUFFIX)

/*
	Extract the list of functions we need wrappers for, pregenerating the wrappers for simple cases:
perl -ne '
if(/^\s*EXPORT\s+(\S+)\s+(mpg123_\S+)\((.*)\);\s*$/)
{
	my $type = $1;
	my $name = $2;
	my $args = $3;
	next unless ($type =~ /off_t/ or $args =~ /off_t/);
	$type =~ s/off_t/long/g;
	my @nargs = ();
	$args =~ s/off_t/long/g;
	foreach my $a (split(/,/, $args))
	{
		$a =~ s/^.*\s\**([a-z_]+)$/$1/;
		push(@nargs, $a);
	}
	my $nargs = join(", ", @nargs);
	$nargs = "Human: figure me out." if($nargs =~ /\(/);
	print <<EOT

$type attribute_align_arg ALIAS_NAME($name)($args)
{
	return $name($nargs);
}
EOT
}' < mpg123.h.in
*/

int attribute_align_arg ALIAS_NAME(mpg123_decode_frame)(mpg123_handle *mh, long *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_decode_frame(mh, num, audio, bytes);
}

int attribute_align_arg ALIAS_NAME(mpg123_framebyframe_decode)(mpg123_handle *mh, long *num, unsigned char **audio, size_t *bytes)
{
	return mpg123_framebyframe_decode(mh, num, audio, bytes);
}

long attribute_align_arg ALIAS_NAME(mpg123_tell)(mpg123_handle *mh)
{
	return mpg123_tell(mh);
}

long attribute_align_arg ALIAS_NAME(mpg123_tellframe)(mpg123_handle *mh)
{
	return mpg123_tellframe(mh);
}

long attribute_align_arg ALIAS_NAME(mpg123_tell_stream)(mpg123_handle *mh)
{
	return mpg123_tell_stream(mh);
}

long attribute_align_arg ALIAS_NAME(mpg123_seek)(mpg123_handle *mh, long sampleoff, int whence)
{
	return mpg123_seek(mh, sampleoff, whence);
}

long attribute_align_arg ALIAS_NAME(mpg123_feedseek)(mpg123_handle *mh, long sampleoff, int whence, long *input_offset)
{
	return mpg123_feedseek(mh, sampleoff, whence, input_offset);
}

long attribute_align_arg ALIAS_NAME(mpg123_seek_frame)(mpg123_handle *mh, long frameoff, int whence)
{
	return mpg123_seek_frame(mh, frameoff, whence);
}

long attribute_align_arg ALIAS_NAME(mpg123_timeframe)(mpg123_handle *mh, double sec)
{
	return mpg123_timeframe(mh, sec);
}

int attribute_align_arg ALIAS_NAME(mpg123_index)(mpg123_handle *mh, long **offsets, long *step, size_t *fill)
{
	return mpg123_index(mh, offsets, step, fill);
}

int attribute_align_arg ALIAS_NAME(mpg123_set_index)(mpg123_handle *mh, long *offsets, long step, size_t fill)
{
	return mpg123_set_index(mh, offsets, step, fill);
}

int attribute_align_arg ALIAS_NAME(mpg123_position)( mpg123_handle *mh, long frame_offset, long buffered_bytes, long *current_frame, long *frames_left, double *current_seconds, double *seconds_left)
{
	return mpg123_position(mh, frame_offset, buffered_bytes, current_frame, frames_left, current_seconds, seconds_left);
}

long attribute_align_arg ALIAS_NAME(mpg123_length)(mpg123_handle *mh)
{
	return mpg123_length(mh);
}

int attribute_align_arg ALIAS_NAME(mpg123_set_filesize)(mpg123_handle *mh, long size)
{
	return mpg123_set_filesize(mh, size);
}

int attribute_align_arg ALIAS_NAME(mpg123_replace_reader)(mpg123_handle *mh, ssize_t (*r_read) (int, void *, size_t), long (*r_lseek)(int, long, int))
{
	return mpg123_replace_reader(mh, r_read, r_lseek);
}

int attribute_align_arg ALIAS_NAME(mpg123_replace_reader_handle)(mpg123_handle *mh, ssize_t (*r_read) (void *, void *, size_t), long (*r_lseek)(void *, long, int), void (*cleanup)(void*))
{
	return mpg123_replace_reader_handle(mh, r_read, r_lseek, cleanup);
}
