/*  XMMS2 plugin for decoding MPEG audio using libmpg123
 *  Copyright (C) 2007-8 Thomas Orgis <thomas@orgis.org>
 *
 *  This is free software under the terms of the LGPL 2.1 .
 *  For libmpg23 API have a look at http://mpg123.org/api/ .
 *
 *  This is also a very basic mpg123 decoder plugin.
 *  Configurable goodies of libmpg123 are missing:
 *   - user decoder choice
 *   - equalizer (fast, using the MPEG frequency bands directly)
 *   - forced mono, resampling (xmms2 can handle that itself, I guess)
 *   - choose RVA preamp (album/mix, from lame tag ReplayGain info or ID3v2 tags)
 *  This should be easy to add for an XMMS2 hacker.
 *
 *  Note on metadata:
 *  With libmpg123 you can get at least all text and comment ID3v2 (version 2.2, 2.3, 2.4)
 *  frames as well as the usual id3v1 info (when you get to the end of the file...).
 *  The decoder also likes to read ID3 tags for getting RVA-related info that players like
 *  foobar2000 store there... Now the problem is: Usually, the id3 xform reads and cuts the id3 data,
 *  Killing the info for mpg123...
 *  Perhaps one can make the generic id3 plugin store the necessary info for retrieval here, or just keep the id3 tags there...
 *  Currently there is no metadata code here, it just _could_ be added.
 */

#include "xmms/xmms_xformplugin.h"
#include "xmms/xmms_log.h"
#include "xmms/xmms_medialib.h"

#include <mpg123.h>
#include <glib.h>
#include <string.h>

/* Just fixing input buffer size here... shall be tunable in future? */
#define BUFSIZE 4096

typedef struct xmms_mpg123_data_St {
	mpg123_handle *decoder;
	mpg123_pars   *param;
	long rate;
	int  channels;
	int  encoding;
	int  alive;
	size_t bps; /* bytes per sample */
	unsigned char buf[BUFSIZE]; /* input data buffer for handing to mpg123 */
	guint64 total_samples;      /* not used yet */
	off_t indata;
} xmms_mpg123_data_t;

static gboolean xmms_mpg123_plugin_setup (xmms_xform_plugin_t *xform_plugin);
static gint     xmms_mpg123_read (xmms_xform_t *xform, xmms_sample_t *buf,
                                 gint len, xmms_error_t *err);
static gboolean xmms_mpg123_init (xmms_xform_t *xform);
static void     xmms_mpg123_destroy (xmms_xform_t *xform);
static gint64   xmms_mpg123_seek (xmms_xform_t *xform, gint64 samples,
                                  xmms_xform_seek_mode_t whence,
                                  xmms_error_t *err);

XMMS_XFORM_PLUGIN ("mpg123", "mpg123 decoder", "0.0",
                   "mpg123 decoder for MPEG 1.0/2.0/2.5 layer 1/2/3 audio",
                   xmms_mpg123_plugin_setup);

static gboolean xmms_mpg123_plugin_setup (xmms_xform_plugin_t *xfp)
{
	xmms_xform_methods_t methods;
	int result;

	result = mpg123_init ();
	if (result != MPG123_OK) return FALSE;
	XMMS_XFORM_METHODS_INIT (methods);
	methods.init    = xmms_mpg123_init;
	methods.destroy = xmms_mpg123_destroy;
	methods.read    = xmms_mpg123_read;
	methods.seek    = xmms_mpg123_seek;
	xmms_xform_plugin_methods_set (xfp, &methods);

	xmms_xform_plugin_indata_add (xfp, XMMS_STREAM_TYPE_MIMETYPE,
	                             "audio/mpeg", /* "audio/x-mpeg", */
	                             NULL);
	/* Well, I usually only see mp3 and mp2 ... layer 1 files are quite rare. */
	xmms_magic_extension_add ("audio/mpeg", "*.mp3");
	xmms_magic_extension_add ("audio/mpeg", "*.mp2");
	xmms_magic_extension_add ("audio/mpeg", "*.mp1");

	/* That's copied from the mad xform. */
	xmms_magic_add ("mpeg header", "audio/mpeg",
	                "0 beshort&0xfff6 0xfff6",
	                "0 beshort&0xfff6 0xfff4",
	                "0 beshort&0xffe6 0xffe2",
	                NULL);

	return TRUE;
}

static gboolean xmms_mpg123_init (xmms_xform_t *xform)
{
	xmms_mpg123_data_t *data;
	int result;
	int i;
	const long *rates;
	size_t num_rates;
	mpg123_rates (&rates, &num_rates);

	g_return_val_if_fail (xform, FALSE);
	data = g_new0 (xmms_mpg123_data_t, 1);
	xmms_xform_private_data_set (xform, data);

	data->indata = 0;
	data->alive  = 1;
	data->param = mpg123_new_pars (&result);
	g_return_val_if_fail (data->param, FALSE);

	/* Create a quiet (stderr) decoder with auto choosen optimization.
	 * Stuff set here should be tunable via plugin config properties!
	 * You can also change some things during playback...
	 */
	mpg123_par (data->param, MPG123_ADD_FLAGS, MPG123_QUIET, 0);
	mpg123_par (data->param, MPG123_ADD_FLAGS, MPG123_GAPLESS, 0);
	/* Enable faster seeking by using estimated / approximate stream offsets. */
	mpg123_par (data->param, MPG123_ADD_FLAGS, MPG123_FUZZY, 0);
	/* choose: MPG123_RVA_OFF, MPG123_RVA_MIX, MPG123_RVA_ALBUM */
	mpg123_par (data->param, MPG123_RVA, MPG123_RVA_ALBUM, 0);
	/* You could choose a decoder from the list provided by
	 * mpg123_supported_decoders () and give that as second parameter.
	 */
	data->decoder = mpg123_parnew (data->param, NULL, &result);
	if (data->decoder == NULL) {
		xmms_log_error ("%s", mpg123_plain_strerror (result));
		goto bad;
	}

	/* Prepare for buffer input feeding. */
	result = mpg123_open_feed (data->decoder);
	if (result != MPG123_OK) {
		goto mpg123_bad;
	}

	/* Let's always decode to signed 16bit for a start.
	   Any mpg123-supported sample rate is accepted. */
	if (MPG123_OK != mpg123_format_none (data->decoder)) {
		goto mpg123_bad;
	}
	for (i=0; i<num_rates; ++i) {
		if (MPG123_OK !=
		    mpg123_format (data->decoder, rates[i], MPG123_MONO|MPG123_STEREO, MPG123_ENC_SIGNED_16)) {
			goto mpg123_bad;
		}
	}
	/* ID3v1 data should be fetched here */
	do {
		/* Parse stream and get info. */
		gint fill;
		xmms_error_t err;
		fill = xmms_xform_read (xform, (gchar*) data->buf, BUFSIZE, &err);
		if (fill <= 0) xmms_log_error ("no input data???");
		else {
			data->indata += fill;
			result = mpg123_decode (data->decoder, data->buf,
			                        (size_t) fill, NULL, 0, NULL);
		}
	} while (result == MPG123_NEED_MORE); /* Keep feeding... */

	if (result != MPG123_NEW_FORMAT) {
		xmms_log_error ("Unable to get the stream going (%s)!",
		                result == MPG123_ERR ? mpg123_strerror (data->decoder)
		                : "though no specific mpg123 error");
		goto bad;
	}

	result = mpg123_getformat (data->decoder,
	                           &data->rate, &data->channels, &data->encoding);
	if (result != MPG123_OK) {
		goto mpg123_bad;
	}

	data->bps = (size_t) data->channels * 2; /* 16bit stereo or mono */
	/* Now it would also be time to get ID3V2/Lame tag data. */
	XMMS_DBG ("mpg123: got stream with %liHz %i channels, encoding %i",
	         data->rate, data->channels, data->encoding);
	xmms_xform_outdata_type_add (xform,
	                             XMMS_STREAM_TYPE_MIMETYPE,
	                             "audio/pcm",
	                             XMMS_STREAM_TYPE_FMT_FORMAT,
	                             XMMS_SAMPLE_FORMAT_S16,
	                             XMMS_STREAM_TYPE_FMT_CHANNELS,
	                             data->channels,
	                             XMMS_STREAM_TYPE_FMT_SAMPLERATE,
	                             (guint) data->rate,
	                             XMMS_STREAM_TYPE_END);
	return TRUE;
mpg123_bad:
	xmms_log_error ("mpg123 error: %s", mpg123_strerror (data->decoder));
bad:
	mpg123_delete (data->decoder);
	mpg123_delete_pars (data->param);
	g_free (data);
	return FALSE;
}

static void xmms_mpg123_destroy (xmms_xform_t *xform)
{
	xmms_mpg123_data_t *data;
	/* Os there a NULL check in this funtion? */
	data = xmms_xform_private_data_get (xform);
	if (data != NULL) {
		mpg123_delete (data->decoder);
		mpg123_delete_pars (data->param);
		g_free (data);
	}
}

static gint xmms_mpg123_read (xmms_xform_t *xform, xmms_sample_t *buf,
                              gint len, xmms_error_t *err)
{
	xmms_mpg123_data_t *data;
	size_t have_read = 0;
	size_t need;
	int result = MPG123_OK;
	g_return_val_if_fail (xform, -1);
	if (len < 1 || buf == NULL) {
		return 0;
	}
	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, -1);
	need = (size_t) len; /* * data->bps; *//* bytes > 0 */

	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, -1);

	do {
		gint fill = 0;
		size_t have_now = 0;
		if (result == MPG123_NEED_MORE) {
			fill = xmms_xform_read (xform, (gchar*) data->buf, BUFSIZE, err);
			if (fill <= 0) {
				if (fill < 0) {
					xmms_log_error ("no input data???");
				}
				if (xmms_xform_iseos (xform)) {
					XMMS_DBG ("apparently this is the end");
				}
				mpg123_decode (data->decoder, data->buf,
				               0, (unsigned char*)buf+have_read,
				               need, &have_now);
				break;
			}
		}
		data->indata += fill;
		result = mpg123_decode (data->decoder, data->buf,
		                        (size_t) fill, (unsigned char*)buf+have_read,
		                        need, &have_now);
		need -= have_now;
		have_read += have_now;
		if (need == 0) return (gint) (have_read);
	} while (result == MPG123_NEED_MORE); /* Keep feeding... */
	if (result == MPG123_NEW_FORMAT) {
		xmms_log_error ("The format changed, cannot handle that.");
		data->alive = 0;
		return 0;
	}
	if (result == MPG123_ERR) {
		xmms_log_error ("mpg123 error: %s", mpg123_strerror (data->decoder));
		return -1;
	}

	return (gint) (have_read);
}

static gint64 xmms_mpg123_seek (xmms_xform_t *xform, gint64 samples,
                                xmms_xform_seek_mode_t whence,
                                xmms_error_t *err)
{
	xmms_mpg123_data_t *data;
	gint64 ret;
	off_t byteoff;
	off_t samploff;
	int mwhence = -1;
	if (whence == XMMS_XFORM_SEEK_SET) mwhence = SEEK_SET;
	else if (whence == XMMS_XFORM_SEEK_CUR) mwhence = SEEK_CUR;
	else if (whence == XMMS_XFORM_SEEK_END) mwhence = SEEK_END;
	XMMS_DBG ("seeking");
	g_return_val_if_fail (xform, -1);
	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, -1);
	/* Get needed input position and possibly reached sample offset from mpg123. */
	samploff = mpg123_feedseek (data->decoder, samples, mwhence, &byteoff);
	XMMS_DBG ("seeked to %li ... intput stream seek following", (long)samploff);
	if (samploff<0)
	{
		xmms_log_error ("mpg123 error: %s", mpg123_strerror (data->decoder));
		return -1;
	}
	/* Seek in input stream. */
	ret = xmms_xform_seek (xform, byteoff, XMMS_XFORM_SEEK_SET, err);
	g_return_val_if_fail (ret != -1, -1);
	return samploff;
}
