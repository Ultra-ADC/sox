#if	defined(ALSA_PLAYER)
/*
 * Copyright 1997 Jimen Ching And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Jimen Ching And Sundry Contributors are not
 * responsible for the consequences of using this software.
 */

/* Direct to ALSA sound driver
 *
 * added by Jimen Ching (jching@flex.com) 19990207
 * based on info grabed from aplay.c in alsa-utils package.
 */

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/asound.h>
#include <sys/ioctl.h>
#include "st.h"
#include "libst.h"

char *get_style(style)
int style;
{
    switch(style)
    {
	case UNSIGNED: return "UNSIGNED";
	case SIGN2: return "SIGNED LINEAR 2's Comp";
	case ULAW: return "uLaw";
	case ALAW: return "aLaw";
	case ADPCM: return "ADPCM";
	default: return "unknown";
    }
}

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *	Find out sampling rate,
 *	size and style of samples,
 *	mono/stereo/quad.
 */
void alsastartread(ft)
ft_t ft;
{
    int bps, fmt, size;
    snd_pcm_record_info_t r_info;
    snd_pcm_format_t format;
    snd_pcm_record_params_t r_params;

    memset(&r_info, 0, sizeof(r_info));
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_RECORD_INFO, &r_info);
    ft->file.count = 0;
    ft->file.pos = 0;
    ft->file.eof = 0;
    ft->file.size = r_info.buffer_size;
    if ((ft->file.buf = malloc (ft->file.size)) == NULL) {
	fail("unable to allocate output buffer of size %d", ft->file.size);
    }
    if (ft->info.rate < r_info.min_rate) ft->info.rate = 2 * r_info.min_rate;
    else if (ft->info.rate > r_info.max_rate) ft->info.rate = r_info.max_rate;
    if (ft->info.channels == -1) ft->info.channels = r_info.min_channels;
    else if (ft->info.channels > r_info.max_channels) ft->info.channels = r_info.max_channels;
    if (ft->info.size == -1) {
	if ((r_info.hw_formats & SND_PCM_FMT_U8) || (r_info.hw_formats & SND_PCM_FMT_S8))
	    ft->info.size = BYTE;
	else
	    ft->info.size = WORD;
    }
    if (ft->info.style == -1) {
	if ((r_info.hw_formats & SND_PCM_FMT_S16_LE) || (r_info.hw_formats & SND_PCM_FMT_S8))
	    ft->info.style = SIGN2;
	else
	    ft->info.style = UNSIGNED;
    }
    if (ft->info.size == BYTE) {
	switch (ft->info.style)
	{
	    case SIGN2:
		if (!(r_info.hw_formats & SND_PCM_FMT_S8))
		    fail("ALSA driver does not support signed byte samples");
		fmt = SND_PCM_SFMT_S8;
		break;
	    case UNSIGNED:
		if (!(r_info.hw_formats & SND_PCM_FMT_U8))
		    fail("ALSA driver does not support unsigned byte samples");
		fmt = SND_PCM_SFMT_U8;
		break;
	    default:
		fail("Hardware does not support %s output", get_style(ft->info.style));
		break;
	}
    }
    else {
	switch (ft->info.style)
	{
	    case SIGN2:
		if (!(r_info.hw_formats & SND_PCM_FMT_S16_LE))
		    fail("ALSA driver does not support signed word samples");
		fmt = SND_PCM_SFMT_S16_LE;
		break;
	    case UNSIGNED:
		if (!(r_info.hw_formats & SND_PCM_FMT_U16_LE))
		    fail("ALSA driver does not support unsigned word samples");
		fmt = SND_PCM_SFMT_U16_LE;
		break;
	    default:
		fail("Hardware does not support %s output", get_style(ft->info.style));
		break;
	}
    }

    memset(&format, 0, sizeof(format));
    format.format = fmt;
    format.rate = ft->info.rate;
    format.channels = ft->info.channels;
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_RECORD_FORMAT, &format);

    size = ft->file.size;
    bps = format.rate * format.channels;
    if (ft->info.size == WORD) bps <<= 1;
    bps >>= 2;
    while (size > bps) size >>= 1;
    if (size < 16) size = 16;
    memset(&r_params, 0, sizeof(r_params));
    r_params.fragment_size = size;
    r_params.fragments_min = 1;
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_RECORD_PARAMS, &r_params);

    /* Change to non-buffered I/O */
    setvbuf(ft->fp, NULL, _IONBF, sizeof(char) * ft->file.size);

    sigintreg(ft);	/* Prepare to catch SIGINT */
}

void alsastartwrite(ft)
ft_t ft;
{
    int bps, fmt, size;
    snd_pcm_playback_info_t p_info;
    snd_pcm_format_t format;
    snd_pcm_playback_params_t p_params;

    memset(&p_info, 0, sizeof(p_info));
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_PLAYBACK_INFO, &p_info);
    ft->file.pos = 0;
    ft->file.eof = 0;
    ft->file.size = p_info.buffer_size;
    if ((ft->file.buf = malloc (ft->file.size)) == NULL) {
	fail("unable to allocate output buffer of size %d", ft->file.size);
    }
    if (ft->info.rate < p_info.min_rate) ft->info.rate = 2 * p_info.min_rate;
    else if (ft->info.rate > p_info.max_rate) ft->info.rate = p_info.max_rate;
    if (ft->info.channels == -1) ft->info.channels = p_info.min_channels;
    else if (ft->info.channels > p_info.max_channels) ft->info.channels = p_info.max_channels;
    if (ft->info.size == -1) {
	if ((p_info.hw_formats & SND_PCM_FMT_U8) || (p_info.hw_formats & SND_PCM_FMT_S8))
	    ft->info.size = BYTE;
	else
	    ft->info.size = WORD;
    }
    if (ft->info.style == -1) {
	if ((p_info.hw_formats & SND_PCM_FMT_S16_LE) || (p_info.hw_formats & SND_PCM_FMT_S8))
	    ft->info.style = SIGN2;
	else
	    ft->info.style = UNSIGNED;
    }
    if (ft->info.size == BYTE) {
	switch (ft->info.style)
	{
	    case SIGN2:
		if (!(p_info.hw_formats & SND_PCM_FMT_S8))
		    fail("ALSA driver does not support signed byte samples");
		fmt = SND_PCM_SFMT_S8;
		break;
	    case UNSIGNED:
		if (!(p_info.hw_formats & SND_PCM_FMT_U8))
		    fail("ALSA driver does not support unsigned byte samples");
		fmt = SND_PCM_SFMT_U8;
		break;
	    default:
		fail("Hardware does not support %s output", get_style(ft->info.style));
		break;
	}
    }
    else {
	switch (ft->info.style)
	{
	    case SIGN2:
		if (!(p_info.hw_formats & SND_PCM_FMT_S16_LE))
		    fail("ALSA driver does not support signed word samples");
		fmt = SND_PCM_SFMT_S16_LE;
		break;
	    case UNSIGNED:
		if (!(p_info.hw_formats & SND_PCM_FMT_U16_LE))
		    fail("ALSA driver does not support unsigned word samples");
		fmt = SND_PCM_SFMT_U16_LE;
		break;
	    default:
		fail("Hardware does not support %s output", get_style(ft->info.style));
		break;
	}
    }

    memset(&format, 0, sizeof(format));
    format.format = fmt;
    format.rate = ft->info.rate;
    format.channels = ft->info.channels;
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_PLAYBACK_FORMAT, &format);

    size = ft->file.size;
    bps = format.rate * format.channels;
    if (ft->info.size == WORD) bps <<= 1;
    bps >>= 2;
    while (size > bps) size >>= 1;
    if (size < 16) size = 16;
    memset(&p_params, 0, sizeof(p_params));
    p_params.fragment_size = size;
    p_params.fragments_max = -1;
    p_params.fragments_room = 1;
    ioctl(fileno(ft->fp), SND_PCM_IOCTL_PLAYBACK_PARAMS, &p_params);

    /* Change to non-buffered I/O */
    setvbuf(ft->fp, NULL, _IONBF, sizeof(char) * ft->file.size);
}

#endif
