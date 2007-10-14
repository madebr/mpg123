#include "mpg123lib_intern.h"

/* static int chans[NUM_CHANNELS] = { 1 , 2 }; */
const long mpg123_rates[MPG123_RATES] = /* only the standard rates */
{
	 8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
};
const int mpg123_encodings[MPG123_ENCODINGS] =
{
	MPG123_ENC_SIGNED_16, 
	MPG123_ENC_UNSIGNED_16,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
};

/*	char audio_caps[NUM_CHANNELS][MPG123_RATES+1][MPG123_ENCODINGS]; */

static int rate2num(mpg123_handle *fr, long r)
{
	int i;
	for(i=0;i<MPG123_RATES;i++) if(mpg123_rates[i] == r) return i;
	if(fr->p.force_rate != 0 && fr->p.force_rate == r) return MPG123_RATES;

	return -1;
}

static int cap_fit(mpg123_handle *fr, struct audioformat *nf, int f0, int f2)
{
	int i;
	int c  = nf->channels-1;
	int rn = rate2num(fr, nf->rate);
	if(rn >= 0)	for(i=f0;i<f2;i++)
	{
		if(fr->p.audio_caps[c][rn][i])
		{
			nf->encoding = mpg123_encodings[i];
			return 1;
		}
	}
	return 0;
}

static int freq_fit(mpg123_handle *fr, struct audioformat *nf, int f0, int f2)
{
	nf->rate = frame_freq(fr)>>fr->p.down_sample;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	nf->rate>>=1;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	nf->rate>>=1;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	return 0;
}

/* match constraints against supported audio formats, store possible setup in frame
  return: -1: error; 0: no format change; 1: format change */
int frame_output_format(mpg123_handle *fr)
{
	struct audioformat nf;
	int f0=0;
	mpg123_pars *p = &fr->p;
	/* initialize new format, encoding comes later */
	nf.channels = fr->stereo;

	if(p->flags & MPG123_FORCE_8BIT) f0 = 2; /* skip the 16bit encodings */

	/* force stereo is stronger */
	if(p->flags & MPG123_FORCE_MONO)   nf.channels = 1;
	if(p->flags & MPG123_FORCE_STEREO) nf.channels = 2;

	if(p->force_rate)
	{
		nf.rate = p->force_rate;
		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,2,MPG123_ENCODINGS)) goto end; /*  8bit encodings */

		/* try again with different stereoness */
		if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
		else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,2,MPG123_ENCODINGS)) goto end; /*  8bit encodings */

		if(NOQUIET)
		error3( "Unable to set up output format! Constraints: %s%s%liHz.",
		        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
		          (p->flags & MPG123_FORCE_MONO ? "mono, " : "") ),
		        (p->flags & MPG123_FORCE_8BIT ? "8bit, " : ""),
		        p->force_rate );
/*		if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

		fr->err = MPG123_BAD_OUTFORMAT;
		return -1;
	}

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf,  2, MPG123_ENCODINGS)) goto end; /* ... 8bit */

	/* try again with different stereoness */
	if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
	else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf,  2, MPG123_ENCODINGS)) goto end; /* ... 8bit */

	/* Here is the _bad_ end. */
	if(NOQUIET)
	error5( "Unable to set up output format! Constraints: %s%s%li, %li or %liHz.",
	        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
	          (p->flags & MPG123_FORCE_MONO ? "mono, "  : "") ),
	        (p->flags & MPG123_FORCE_8BIT  ? "8bit, " : ""),
	        frame_freq(fr),  frame_freq(fr)>>1, frame_freq(fr)>>2 );
/*	if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

	fr->err = MPG123_BAD_OUTFORMAT;
	return -1;

end: /* Here is the _good_ end. */
	/* we had a successful match, now see if there's a change */
	if(nf.rate == fr->af.rate && nf.channels == fr->af.channels && nf.encoding == fr->af.encoding)
	return 0; /* the same format as before */
	else /* a new format */
	{
		fr->af.rate = nf.rate;
		fr->af.channels = nf.channels;
		fr->af.encoding = nf.encoding;
		return 1;
	}
}

int mpg123_getformat(mpg123_handle *mh, long *rate, int *channels, int *encoding)
{
	if(mh == NULL) return MPG123_ERR;
	*rate = mh->af.rate;
	*channels = mh->af.channels;
	*encoding = mh->af.encoding;
	return MPG123_OK;
}

int mpg123_format_none(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;
	memset(mh->p.audio_caps,0,sizeof(mh->p.audio_caps));
	return MPG123_OK;
}

int mpg123_format_all(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;
	memset(mh->p.audio_caps,1,sizeof(mh->p.audio_caps));
	return MPG123_OK;
}

int mpg123_format(mpg123_handle *mh, int ratei, int channels, int encodings)
{
	int ie, ic;
	int ch[2] = {0, 1};
	if(!(channels & (MPG123_MONO|MPG123_STEREO)))
	{
		mh->err = MPG123_BAD_CHANNEL;
		return MPG123_ERR;
	}
	if(!(channels & MPG123_STEREO)) ch[1] = 0;     /* {0,0} */
	else if(!(channels & MPG123_MONO)) ch[0] = 1; /* {1,1} */
	if(ratei >= MPG123_RATES)
	{
		mh->err = MPG123_BAD_RATE;
		return MPG123_ERR;
	}
	if(ratei < 0) ratei = MPG123_RATES; /* the special one */

	/* now match the encodings */
	for(ic = 0; ic < 2; ++ic)
	{
		for(ie = 0; ie < MPG123_ENCODINGS; ++ie)
		if(mpg123_encodings[ie] & encodings) mh->p.audio_caps[ch[ic]][ratei][ie] = 1;

		if(ch[0] == ch[1]) break; /* no need to do it again */
	}

	return MPG123_OK;
}

int mpg123_format_support(mpg123_handle *mh, int ratei, int enci)
{
	int ch = 0;
	if(mh == NULL || ratei >= MPG123_RATES || enci < 0 || enci >= MPG123_ENCODINGS) return 0;
	if(ratei < 0) ratei = MPG123_RATES; /* the special one */
	if(mh->p.audio_caps[0][ratei][enci]) ch |= MPG123_MONO;
	if(mh->p.audio_caps[1][ratei][enci]) ch |= MPG123_STEREO;
	return ch;
}
