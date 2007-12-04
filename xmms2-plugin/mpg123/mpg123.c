/*  XMMS2 plugin for decoding MPEG audio using libmpg123
 *  Copyright (C) 2007 Thomas Orgis <thomas@orgis.org>
 *
 *  This is free software under the terms of the LGPL 2.1 .
 *
 *  This is also a very basic mpg123 decoder plugin.
 *  Configurable goodies of libmpg123 are missing:
 *   - user decoder choice
 *   - equalizer (_fast!_)
 *   - forced mono, resampling
 *  Some basic metadata is read and set in the strem properties.
 *  With libmpg123 you can get at least all text and comment ID3v2 (version 2.2, 2.3, 2.4)
 *  frames as well as the usual id3v1 info (when you get to the end of the file...).
 *  The decoder also likes to read ID3 tags for getting RVA-related info that players like
 *  foobar2000 store there... Now the problem is: Usually, the id3 xform reads and cuts the id3 data,
 *  Killing the info for mpg123...
 */

#include "xmms/xmms_xformplugin.h"
#include "xmms/xmms_log.h"
#include "xmms/xmms_medialib.h"

#include <mpg123.h>
#include <glib.h>
#include <string.h>

/* Just fixing it here... shall be tunable in future? */
#define BUFSIZE 4096

typedef struct xmms_mpg123_data_St {
	mpg123_handle *decoder;
	mpg123_pars   *param;
	long rate;
	int  channels;
	int  encoding;
	int  alive;
	size_t bps; /* bytes per sample */
	unsigned char buf[BUFSIZE]; /* hm, what kind of 8bit char??? */
	guint64 total_samples;      /* not used yet */
	off_t indata;
} xmms_mpg123_data_t;

static gboolean xmms_mpg123_plugin_setup(xmms_xform_plugin_t *xform_plugin);
static gint     xmms_mpg123_read(xmms_xform_t *xform, xmms_sample_t *buf,
                                 gint len, xmms_error_t *err);
static gboolean xmms_mpg123_init(xmms_xform_t *xform);
static void     xmms_mpg123_destroy(xmms_xform_t *xform);
static gint64   xmms_mpg123_seek(xmms_xform_t *xform, gint64 samples,
                                 xmms_xform_seek_mode_t whence,
                                 xmms_error_t *err);

XMMS_XFORM_PLUGIN("mpg123", "mpg123 decoder", "0.0",
                  "mpg123 decoder for MPEG 1.0/2.0/2.5 layer 1/2/3 audio",
                  xmms_mpg123_plugin_setup);

static gboolean xmms_mpg123_plugin_setup(xmms_xform_plugin_t *xfp)
{
	xmms_xform_methods_t methods;
	int result;

	XMMS_DBG("setup1");
	result = mpg123_init();
	if(result != MPG123_OK) return FALSE;
	XMMS_DBG("setup2");
	XMMS_XFORM_METHODS_INIT (methods);
	methods.init    = xmms_mpg123_init;
	methods.destroy = xmms_mpg123_destroy;
	methods.read    = xmms_mpg123_read;
	methods.seek    = xmms_mpg123_seek;
	xmms_xform_plugin_methods_set(xfp, &methods);

	XMMS_DBG("setup3");
	xmms_xform_plugin_indata_add(xfp, XMMS_STREAM_TYPE_MIMETYPE,
	                             "audio/mpeg", /* "audio/x-mpeg", */
	                             NULL);
	XMMS_DBG("setup3a");
	/* Well, I usually only see mp3 and mp2 ... layer 1 files are quite rare. */
	xmms_magic_extension_add("audio/mpeg", "*.mp3");
	XMMS_DBG("setup3b");
	xmms_magic_extension_add("audio/mpeg", "*.mp2");
	XMMS_DBG("setup3c");
	xmms_magic_extension_add("audio/mpeg", "*.mp1");

	/* Not so sure about magic numbers for metadata-infested mpeg audio... */
	XMMS_DBG("setup4");

	return TRUE;
}

/* Update metadata... fail silently. */
static void xmms_mpg123_metacheck(xmms_xform_t *xform)
{
	xmms_mpg123_data_t *data;
	int mc;
	if(xform == NULL) return;
	data = xmms_xform_private_data_get(xform);
	if(data == NULL) return;
	XMMS_DBG("check for new metadata");
	mc = mpg123_meta_check(data->decoder);
	XMMS_DBG("returned 0x%x", mc);
	if(mc & MPG123_NEW_ID3)
	{
		XMMS_DBG("got new ID3v2 data");
		mpg123_id3v2 *tag;
		mpg123_id3(data->decoder, NULL, &tag);
		if(tag->title != NULL && tag->title->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_TITLE, tag->title->p);
		}
		if(tag->artist != NULL && tag->artist->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_ARTIST, tag->artist->p);
		}
		if(tag->album != NULL && tag->album->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_ALBUM, tag->album->p);
		}
		if(tag->year != NULL && tag->year->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_YEAR, tag->year->p);
		}
		if(tag->comment != NULL && tag->comment->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_COMMENT, tag->comment->p);
		}
		/* Well... genre needs postprocessing... numbers from id3 table perhaps. */
		if(tag->genre != NULL && tag->genre->fill > 0) {
			xmms_xform_metadata_set_str(xform, XMMS_MEDIALIB_ENTRY_PROPERTY_GENRE, tag->genre->p);
		}
	}
}

static gboolean xmms_mpg123_init(xmms_xform_t *xform)
{
	xmms_mpg123_data_t *data;
	int result;
	int i;
	const long *rates;
	size_t num_rates;
	mpg123_rates(&rates, &num_rates);

	XMMS_DBG("init");
	g_return_val_if_fail(xform, FALSE);
	data = g_new0(xmms_mpg123_data_t, 1);
	xmms_xform_private_data_set(xform, data);
	XMMS_DBG("init2");

	data->indata = 0;
	data->alive  = 1;
	data->param = mpg123_new_pars(&result);
	g_return_val_if_fail(data->param, FALSE);
	XMMS_DBG("init3");

	/* Create a quiet (stderr) decoder with auto choosen optimization. */
	/* Stuff set here should be tunable via plugin config properties. */
	mpg123_par(data->param, MPG123_ADD_FLAGS, MPG123_QUIET, 0);
	mpg123_par(data->param, MPG123_ADD_FLAGS, MPG123_GAPLESS, 0);
	/* choose: MPG123_RVA_OFF, MPG123_RVA_MIX, MPG123_RVA_ALBUM */
	mpg123_par(data->param, MPG123_RVA, MPG123_RVA_ALBUM, 0);
	data->decoder = mpg123_parnew(NULL, NULL, &result);
	if(data->decoder == NULL) {
		xmms_log_fatal("%s", mpg123_plain_strerror(result));
		goto bad;
	}

	XMMS_DBG("init4");
	/* Prepare for buffer input feeding. */
	result = mpg123_open_feed(data->decoder);
	if(result != MPG123_OK) {
		goto mpg123_bad;
	}

	XMMS_DBG("init5");
	/* Let's always decode to signed 16bit for a start.
	   Any mpg123-supported sample rate is accepted. */
	if(MPG123_OK != mpg123_format_none(data->decoder)) {
		goto mpg123_bad;
	}
	for(i=0; i<num_rates; ++i) {
		if(MPG123_OK !=
		   mpg123_format(data->decoder, rates[i], MPG123_MONO|MPG123_STEREO, MPG123_ENC_SIGNED_16)) {
			goto mpg123_bad;
		}
	}
	XMMS_DBG("init6");
	/* ID3v1 data should be fetched here */
	do {
		/* Parse stream and get info. */
		gint fill;
		xmms_error_t err;
		fill = xmms_xform_read(xform, (gchar*) data->buf, BUFSIZE, &err);
		if(fill < 0) xmms_log_error("no input data???");
		else {
			data->indata += fill;
			size_t fakegot; /* Not actually getting anything... */
			result = mpg123_decode(data->decoder, data->buf,
			                       (size_t) fill, NULL, 0, &fakegot);
		}
	} while(result == MPG123_NEED_MORE); /* Keep feeding... */
	XMMS_DBG("init7");
	if(result != MPG123_NEW_FORMAT) {
		xmms_log_fatal("Unable to get the stream going (%s)!",
		               result == MPG123_ERR ? mpg123_strerror(data->decoder)
		               : "though no specific mpg123 error");
		goto bad;
	}

	xmms_mpg123_metacheck(xform);
	result = mpg123_getformat(data->decoder,
	                          &data->rate, &data->channels, &data->encoding);
	XMMS_DBG("init8");
	if(result != MPG123_OK) {
		goto mpg123_bad;
	}

	data->bps = (size_t) data->channels * 2; /* 16bit stereo or mono */
	/* Now it would also be time to get ID3V2/Lame tag data. */
	XMMS_DBG("mpg123: got stream with %liHz %i channels, encoding %i",
	         data->rate, data->channels, data->encoding);
	xmms_xform_outdata_type_add(xform,
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
	xmms_log_fatal("mpg123 error: %s", mpg123_strerror(data->decoder));
bad:
	mpg123_delete(data->decoder);
	mpg123_delete_pars(data->param);
	g_free(data);
	return FALSE;
}

static void xmms_mpg123_destroy(xmms_xform_t *xform)
{
	xmms_mpg123_data_t *data;
	/* Os there a NULL check in this funtion? */
	data = xmms_xform_private_data_get(xform);
	if(data != NULL) {
		mpg123_delete(data->decoder);
		mpg123_delete_pars(data->param);
		g_free(data);
	}
}

static gint xmms_mpg123_read(xmms_xform_t *xform, xmms_sample_t *buf,
                             gint len, xmms_error_t *err)
{
	xmms_mpg123_data_t *data;
	size_t have_read = 0;
	size_t need;
	int result = MPG123_OK;
	g_return_val_if_fail(xform, -1);
	if(len < 1 || buf == NULL) {
		return 0;
	}
	data = xmms_xform_private_data_get(xform);
	g_return_val_if_fail(data, -1);
	need = (size_t) len; /* * data->bps; *//* bytes > 0 */

	data = xmms_xform_private_data_get(xform);
	g_return_val_if_fail(data, -1);

	do {
		gint fill = 0;
		size_t have_now = 0;
		if(result == MPG123_NEED_MORE) {
			fill = xmms_xform_read(xform, (gchar*) data->buf, BUFSIZE, err);
			if(fill <= 0) {
				if(fill < 0) {
					xmms_log_error("no input data???");
				}
				if(xmms_xform_iseos(xform)) {
					XMMS_DBG("apparently this is the end");
				}
				mpg123_decode(data->decoder, data->buf,
		                       0, (unsigned char*)buf+have_read,
		                       need, &have_now);
				break;
			}
			XMMS_DBG("%lu bytes input (total pos %lu)", (unsigned long) fill, (unsigned long)data->indata);
		}
		data->indata += fill;
		result = mpg123_decode(data->decoder, data->buf,
		                       (size_t) fill, (unsigned char*)buf+have_read,
		                       need, &have_now);
		need -= have_now;
		have_read += have_now;
		/* Live update of metadata (multiple tracks in one stream)? */
		/* xmms_mpg123_metacheck(xform); */
		if(need == 0) return (gint)(have_read); /*/data->bps);*/
	} while(result == MPG123_NEED_MORE); /* Keep feeding... */
	if(result == MPG123_NEW_FORMAT) {
		xmms_log_error("The format changed, cannot handle that.");
		data->alive = 0;
		return 0;
	}
	if(result == MPG123_ERR) {
		xmms_log_error("mpg123 error: %s", mpg123_strerror(data->decoder));
		return -1;
	}
	XMMS_DBG("%lu bytes in total, %lu bytes out", (unsigned long)data->indata, (unsigned long)have_read);
	return (gint)(have_read); /*/data->bps);*/
}

static gint64 xmms_mpg123_seek(xmms_xform_t *xform, gint64 samples,
                               xmms_xform_seek_mode_t whence,
                               xmms_error_t *err)
{
	xmms_mpg123_data_t *data;
	off_t byteoff;
	off_t samploff;
	int mwhence = -1;
	if(whence == XMMS_XFORM_SEEK_SET) mwhence = SEEK_SET;
	else if(whence == XMMS_XFORM_SEEK_CUR) mwhence = SEEK_CUR;
	else if(whence == XMMS_XFORM_SEEK_END) mwhence = SEEK_END;
	XMMS_DBG("seeking");
	g_return_val_if_fail(xform, -1);
	data = xmms_xform_private_data_get(xform);
	g_return_val_if_fail(data, -1);
	samploff = mpg123_feedseek(data->decoder, samples, mwhence, &byteoff);
	XMMS_DBG("seeked to %li ... intput stream seek following", (long)samploff);
	if(samploff<0)
	{
		xmms_log_error("mpg123 error: %s", mpg123_strerror(data->decoder));
		return -1;
	}
	g_return_val_if_fail(xmms_xform_seek(xform, byteoff, XMMS_XFORM_SEEK_SET, err) != -1, -1);
	return samploff;
}
