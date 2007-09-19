/*
 *  Driver for Advanced Linux Sound Architecture, http://alsa.jcu.cz
 *  This driver is for the old (version 0.5-0.8) ALSA API
 * 
 *  Code by Anders Semb Hermansen <ahermans@vf.telia.no>
 *  Cleanups by Jaroslav Kysela <perex@jcu.cz>
 *              Ville Syrjala <syrjala@sci.fi>
 *
 *  You can use -a <soundcard #>:<device #>...
 *  For example: mpg123 -a 1:0 aaa.mpg
 *               mpg123 -a guspnp:1 aaa.mpg
 *
 * This file comes under GPL license, version 2. 
 */

#include "mpg123.h"

#include <ctype.h>
#include <sys/asoundlib.h>

#ifdef SND_LITTLE_ENDIAN
#define SND_PCM_SFMT_S16_NE SND_PCM_SFMT_S16_LE
#define SND_PCM_SFMT_U16_NE SND_PCM_SFMT_U16_LE
#define SND_PCM_FMT_S16_NE SND_PCM_FMT_S16_LE
#define SND_PCM_FMT_U16_NE SND_PCM_FMT_U16_LE
#else
#define SND_PCM_SFMT_S16_NE SND_PCM_SFMT_S16_BE
#define SND_PCM_SFMT_U16_NE SND_PCM_SFMT_U16_BE
#define SND_PCM_FMT_S16_NE SND_PCM_FMT_S16_BE
#define SND_PCM_FMT_U16_NE SND_PCM_FMT_U16_BE
#endif




static void set_playback_format(audio_output_t *ao)
{
	snd_pcm_format_t alsa_format;
	int err;
	
	alsa_format.rat = ao->rate;
	alsa_format.channels = ao->channels;

	switch(ao->format)
	{
		case AUDIO_FORMAT_SIGNED_16:
		default:
			ao->alsa_format.format=SND_PCM_SFMT_S16_NE;
			break;
		case AUDIO_FORMAT_UNSIGNED_8:
			ao->alsa_format.format=SND_PCM_SFMT_U8;
			break;
		case AUDIO_FORMAT_SIGNED_8:
			ao->alsa_format.format=SND_PCM_SFMT_S8;
			break;
		case AUDIO_FORMAT_ULAW_8:
			ao->alsa_format.format=SND_PCM_SFMT_MU_LAW;
			break;
		case AUDIO_FORMAT_ALAW_8:
			ao->alsa_format.format=SND_PCM_SFMT_A_LAW;
			break;
		case AUDIO_FORMAT_UNSIGNED_16:
			ao->alsa_format.format=SND_PCM_SFMT_U16_NE;
			break;
	}

	if((err=snd_pcm_playback_format(ao->handle, &alsa_format)) < 0 )
	{
		error1("snd_pcm_playback_format failed: %s", snd_strerror(err));
		exit(1);
	}
}


static void audio_set_playback_params(audio_output_t *ao)
{
	int err;
	snd_pcm_playback_info_t pi;
	snd_pcm_playback_params_t pp;

	if((err=snd_pcm_playback_info(ao->handle, &pi)) < 0 )
	{
		error1("playback info failed: %s", snd_strerror(err));
		return;	/* not fatal error */
	}

	bzero(&pp, sizeof(pp));
	pp.fragment_size = pi.buffer_size/4;
	if (pp.fragment_size > pi.max_fragment_size) pp.fragment_size = pi.max_fragment_size;
	if (pp.fragment_size < pi.min_fragment_size) pp.fragment_size = pi.min_fragment_size;
	pp.fragments_max = -1;
	pp.fragments_room = 1;

	if((err=snd_pcm_playback_params(ao->handle, &pp)) < 0 )
	{
		erorr1("playback params failed: %s", snd_strerror(err));
		return; /* not fatal error */
	}
}


static int open_alsa05(audio_output_t *ao)
{
	int err;
	int card=0,device=0;
	char scard[128], sdevice[128];

	if(!ai)
		return -1;
		
	if(ao->device) {	/* parse ALSA device name */
		if(strchr(ao->device,':')) {	/* card with device */
			strncpy(scard, ao->device, sizeof(scard)-1);
			scard[sizeof(scard)-1] = '\0';
			if (strchr(scard,':')) *strchr(scard,':') = '\0';
			card = snd_card_name(scard);
			if (card < 0) {
				error1("wrong soundcard number: %s", scard);
				exit(1);
			}
			strncpy(sdevice, strchr(ao->device, ':') + 1, sizeof(sdevice)-1);
		} else {
			strncpy(sdevice, ao->device, sizeof(sdevice)-1);
		}
		sdevice[sizeof(sdevice)-1] = '\0';
		device = atoi(sdevice);
		if (!isdigit(sdevice[0]) || device < 0 || device > 31) {
			error1("wrong device number: %s", sdevice);
			exit(1);
		}
	}

	// Open the ALSA device
	if((err=snd_pcm_open(&ao->handle, card, device, SND_PCM_OPEN_PLAYBACK)) < 0 )
	{
		error1("open failed: %s", snd_strerror(err));
		exit(1);
	}


	// Now configure the device
	set_playback_format( ai )
	set_playback_params( ai );


	return 0;
}





static int get_formats_alsa05(audio_output_t *ao)
{
	int i, err;
	int fmt = -1;
	snd_pcm_playback_info_t pi;

	static int fmts[] = {
		AUDIO_FORMAT_SIGNED_16, AUDIO_FORMAT_UNSIGNED_16,
		AUDIO_FORMAT_UNSIGNED_8, AUDIO_FORMAT_SIGNED_8,
		AUDIO_FORMAT_ULAW_8, AUDIO_FORMAT_ALAW_8
	};
	static int afmts[] = {
		SND_PCM_FMT_S16_NE, SND_PCM_FMT_U16_NE,
		SND_PCM_FMT_U8, SND_PCM_FMT_S8,
		SND_PCM_FMT_MU_LAW, SND_PCM_FMT_A_LAW
	};

	if((err=snd_pcm_playback_info(ao->handle, &pi)) < 0 )
	{
		error1("playback info failed: %s", snd_strerror(err));
		return -1;
	}

	for (i = 0; i < 6; i++) {
		if (pi.formats & afmts[i]) {
			if (fmt == -1)
				fmt = 0;
			fmt |= fmts[i];
		}
	}

	return fmt;
}

static int write_alsa05(audio_output_t *ao,unsigned char *buf,int len)
{
	ssize_t ret;

	ret=snd_pcm_write(ao->handle, buf, len);

	return ret;
}

static int close_alsa05(audio_output_t *ao)
{
	int ret;
	ret = snd_pcm_close(ao->handle);
	return ret;
}

static void flush_alsa05(audio_output_t *ao)
{
}


static int init_alsa05(audio_output_t* ao)
{
	if (ao==NULL) return -1;

	/* Set callbacks */
	ao->open = open_alsa05;
	ao->flush = flush_alsa05;
	ao->write = write_alsa05;
	ao->get_formats = get_formats_alsa05;
	ao->close = close_alsa05;

	/* Allocate memory for data structure */
	/* Success */
	return 0;
}



/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"alsa05",						
	/* description */	"Output audio using old (version 0.5-0.8) ALSA API.",
	/* revision */		"$Rev:$",
	/* handle */		NULL,
	
	/* init_output */	init_alsa05,						
};


