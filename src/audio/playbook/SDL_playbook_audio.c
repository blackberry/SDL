/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output raw audio data to a file. */

#include <stdio.h>
#include <stdlib.h>

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_playbook_audio.h"

static void DEBUG_doNothing (FILE * fd, char *format, ...) {
      va_list ap;
      va_start(ap, format);
      va_end(ap);
}


#define DEBUG_printf DEBUG_doNothing
//#define DEBUG_printf fprintf

//#define ACTUALLY_DO_DISK

/* The tag name used by Anthony's asound audio */
#define PLAYBOOK_AUD_DRIVER_NAME         "anhu"

/* environment variables and defaults. */

#define PLAYBOOK_DEFAULT_WRITEDELAY   150

/* Audio driver functions */
static int PLAYBOOK_AUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void PLAYBOOK_AUD_WaitAudio(_THIS);
static void PLAYBOOK_AUD_PlayAudio(_THIS);
static Uint8 *PLAYBOOK_AUD_GetAudioBuf(_THIS);
static void PLAYBOOK_AUD_CloseAudio(_THIS);

#ifdef ACTUALLY_DO_DISK
static const char *PLAYBOOK_AUD_GetOutputFilename(void)
{
    return "/sdlaudio.raw";
}
#endif
/* Audio driver bootstrap functions */
static int PLAYBOOK_AUD_Available(void)
{
    return(1);
}


static void PLAYBOOK_AUD_DeleteDevice(SDL_AudioDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_AudioDevice *PLAYBOOK_AUD_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;
    const char *envr;

    /* Initialize all variables that we clean on shutdown */
    this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
    if ( this ) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)
                SDL_malloc((sizeof *this->hidden));
    }

    if ( (this == NULL) || (this->hidden == NULL) ) {
        SDL_OutOfMemory();
        if ( this ) {
            SDL_free(this);
        }
        return(0);
    }

    SDL_memset(this->hidden, 0, (sizeof *this->hidden));
    this->hidden->write_delay = PLAYBOOK_DEFAULT_WRITEDELAY;

    /* Set the function pointers */
    this->OpenAudio = PLAYBOOK_AUD_OpenAudio;
    this->WaitAudio = PLAYBOOK_AUD_WaitAudio;
    this->PlayAudio = PLAYBOOK_AUD_PlayAudio;
    this->GetAudioBuf = PLAYBOOK_AUD_GetAudioBuf;
    this->CloseAudio = PLAYBOOK_AUD_CloseAudio;

    this->free = PLAYBOOK_AUD_DeleteDevice;

    return this;
}

AudioBootStrap PLAYBOOK_AUD_bootstrap = {
        PLAYBOOK_AUD_DRIVER_NAME, "Anthony's libasound audio",
        PLAYBOOK_AUD_Available, PLAYBOOK_AUD_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void PLAYBOOK_AUD_WaitAudio(_THIS)
{
    fd_set  rfds, wfds;
    int nflds;
    int readAvailable = 1;
    int writeAvailable = 0;
#ifdef ACTUALLY_DO_DISK
    SDL_Delay(this->hidden->write_delay);
#endif

    DEBUG_printf(stderr, "WaitAudio called\n");
    FD_ZERO (&rfds);
    FD_ZERO (&wfds);

    FD_SET (snd_mixer_file_descriptor (this->hidden->mixer_handle), &rfds);
    FD_SET (snd_pcm_file_descriptor (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds);

    nflds = max (snd_mixer_file_descriptor (this->hidden->mixer_handle), snd_pcm_file_descriptor (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK));

    if (select (nflds + 1, &rfds, &wfds, NULL, NULL) == -1) {
        DEBUG_printf(stderr, "WaitAudio: select failure\n");
        return;
    }

    while (!readAvailable && writeAvailable) {

        /* seems pointless....
         * TODO: consider taking this stuff out.
         */
        if (FD_ISSET (snd_mixer_file_descriptor (this->hidden->mixer_handle), &rfds)) {
            snd_mixer_callbacks_t callbacks = { 0, 0, 0, 0 };
            snd_mixer_read (this->hidden->mixer_handle, &callbacks);
        }

        FD_SET (snd_mixer_file_descriptor (this->hidden->mixer_handle), &rfds);
        FD_SET (snd_pcm_file_descriptor (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds);

        readAvailable = FD_ISSET(snd_mixer_file_descriptor (this->hidden->mixer_handle), &rfds);
        writeAvailable = FD_ISSET (snd_pcm_file_descriptor (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds);
        DEBUG_printf(stderr, "waiting...\n");
    }

}

static void PLAYBOOK_AUD_PlayAudio(_THIS)
{
    int written;
    fd_set  wfds;
    DEBUG_printf(stderr, "requested to write %d bytes of audio data\n", this->hidden->mixlen);

#ifdef ACTUALLY_DO_DISK
    /* Write the audio data */
    written = SDL_RWwrite(this->hidden->output,
                        this->hidden->mixbuf, 1,
                        this->hidden->mixlen);

    /* If we couldn't write, assume fatal error for now */
    if ( (Uint32)written != this->hidden->mixlen ) {
        DEBUG_printf (stderr, "write error (%d bytes written)\n", written);
        this->enabled = 0;
    }
#endif

    FD_ZERO (&wfds);

    if (FD_ISSET (snd_pcm_file_descriptor (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds)) {
        DEBUG_printf(stderr, "nothing to write!!");
        return;
    }

    written = snd_pcm_plugin_write (this->hidden->pcm_handle, this->hidden->mixbuf, this->hidden->mixlen);

    if (written == this->hidden->mixlen) {
        DEBUG_printf (stderr, "bytes written as expected (%d bytes)\n", written);
    } else {
        DEBUG_printf (stderr, "write underflow (%d bytes)\n", written);
    }
}

static Uint8 *PLAYBOOK_AUD_GetAudioBuf(_THIS)
{
    return(this->hidden->mixbuf);
}

static void PLAYBOOK_AUD_CloseAudio(_THIS)
{
    snd_pcm_plugin_flush (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK);
    snd_mixer_close (this->hidden->mixer_handle);
    snd_pcm_close (this->hidden->pcm_handle);


    if ( this->hidden->mixbuf != NULL ) {
        SDL_FreeAudioMem(this->hidden->mixbuf);
        this->hidden->mixbuf = NULL;
    }

#ifdef ACTUALLY_DO_DISK
    if ( this->hidden->output != NULL ) {
        SDL_RWclose(this->hidden->output);
        this->hidden->output = NULL;
    }
#endif
}

static int PLAYBOOK_AUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{

#ifdef ACTUALLY_DO_DISK
    const char *fname = PLAYBOOK_AUD_GetOutputFilename();
#endif

    int     card = -1;
    int     dev = 0;
    FILE   *file1;
    int     mSamples;
    int     mSampleRate = spec->freq;
    int     mSampleChannels = spec->channels;
    int     mSampleBits;
    char   *mSampleBfr1;
    int     fragsize = -1;
    int     verbose = 0;

    int     rtn;
    snd_pcm_channel_info_t pi;
    snd_mixer_group_t group;
    snd_pcm_channel_params_t pp;
    snd_pcm_channel_setup_t setup;
    int     bsize, n, N = 0, c;
    uint32_t voice_mask[] = { 0, 0, 0, 0 };
    snd_pcm_voice_conversion_t voice_conversion;
    int     voice_override = 0;
    int     num_frags = -1;
    char   *sub_opts, *value;

    DEBUG_printf(stderr, "WARNING: You are using Anthony's SDL playbook hack audio driver\n");

#ifdef ACTUALLY_DO_DISK
    /* Open the audio device */
    this->hidden->output = SDL_RWFromFile(fname, "wb");
    if ( this->hidden->output == NULL ) {
        return(-1);
    }
    DEBUG_printf(stderr, "Actually writing to file [%s].\n", fname);
#endif


    /* Allocate mixing buffer */
    this->hidden->mixlen = spec->size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if ( this->hidden->mixbuf == NULL ) {
        return(-1);
    }
    SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);


    //below here is where we actually do pb stuff.
    // FIXME: Use SDL_SetError to record errors instead of DEBUG_printf.
    if ((rtn = snd_pcm_open_preferred(&this->hidden->pcm_handle, &card, &dev, SND_PCM_OPEN_PLAYBACK)) < 0)
    {
        DEBUG_printf (stderr, "snd_pcm_open_preferred failed: %s\n", snd_strerror(rtn));
        return -1;
    }

    /* disabling mmap is not actually required in this example but it is included to
         * demonstrate how it is used when it is required.
         */
    if ((rtn = snd_pcm_plugin_set_disable (this->hidden->pcm_handle, PLUGIN_DISABLE_MMAP)) < 0)
    {
        DEBUG_printf (stderr, "snd_pcm_plugin_set_disable failed: %s\n", snd_strerror (rtn));
        return -1;
    }

    memset (&pi, 0, sizeof (pi));
    pi.channel = SND_PCM_CHANNEL_PLAYBACK;
    if ((rtn = snd_pcm_plugin_info (this->hidden->pcm_handle, &pi)) < 0)
    {
        DEBUG_printf (stderr, "snd_pcm_plugin_info failed: %s\n", snd_strerror (rtn));
        return -1;
    }

    memset (&pp, 0, sizeof (pp));

    pp.mode = SND_PCM_MODE_BLOCK;
    pp.channel = SND_PCM_CHANNEL_PLAYBACK;
    pp.start_mode = SND_PCM_START_FULL;
    pp.stop_mode = SND_PCM_STOP_STOP;

    pp.buf.block.frag_size = pi.max_fragment_size;
    if (fragsize != -1)
    {
        pp.buf.block.frag_size = fragsize;
    }
    pp.buf.block.frags_max = num_frags;
    pp.buf.block.frags_min = 1;

    pp.format.interleave = 1;
    pp.format.rate = mSampleRate;
    pp.format.voices = mSampleChannels;

    if (spec->format == AUDIO_U8)
        pp.format.format = SND_PCM_SFMT_U8;
    else if (spec->format == AUDIO_S8)
       pp.format.format = SND_PCM_SFMT_S8;
    else if (spec->format == AUDIO_U16LSB)
        pp.format.format = SND_PCM_SFMT_U16_LE;
    else if (spec->format == AUDIO_S16LSB)
        pp.format.format = SND_PCM_SFMT_S16_LE;
    else if (spec->format == AUDIO_U16MSB)
        pp.format.format = SND_PCM_SFMT_U16_BE;
    else if (spec->format == AUDIO_S16MSB)
        pp.format.format = SND_PCM_SFMT_S16_BE;
    else {
        DEBUG_printf (stderr, "we don't want to support this sound format: %x \n", this->spec.format);
        return -1;
    }

    strcpy (pp.sw_mixer_subchn_name, "Wave playback channel");
    if ((rtn = snd_pcm_plugin_params (this->hidden->pcm_handle, &pp)) < 0)
    {
        DEBUG_printf (stderr, "snd_pcm_plugin_params failed: %s\n", snd_strerror (rtn));
        return -1;
    }

    if ((rtn = snd_pcm_plugin_prepare (this->hidden->pcm_handle, SND_PCM_CHANNEL_PLAYBACK)) < 0)
        DEBUG_printf (stderr, "snd_pcm_plugin_prepare failed: %s\n", snd_strerror (rtn));

    memset (&setup, 0, sizeof (setup));
    memset (&group, 0, sizeof (group));
    setup.channel = SND_PCM_CHANNEL_PLAYBACK;
    setup.mixer_gid = &group.gid;
    if ((rtn = snd_pcm_plugin_setup (this->hidden->pcm_handle, &setup)) < 0)
    {
        DEBUG_printf (stderr, "snd_pcm_plugin_setup failed: %s\n", snd_strerror (rtn));
        return -1;
    }
    DEBUG_printf (stderr, "Format %s \n", snd_pcm_get_format_name (setup.format.format));
    DEBUG_printf (stderr, "Frag Size %d \n", setup.buf.block.frag_size);
    DEBUG_printf (stderr, "Total Frags %d \n", setup.buf.block.frags);
    DEBUG_printf (stderr, "Rate %d \n", setup.format.rate);
    DEBUG_printf (stderr, "Voices %d \n", setup.format.voices);
    bsize = setup.buf.block.frag_size;

    if (group.gid.name[0] == 0)
    {
        DEBUG_printf (stderr, "Mixer Pcm Group [%s] Not Set \n", group.gid.name);
        exit (-1);
    }
    DEBUG_printf (stderr, "Mixer Pcm Group [%s]\n", group.gid.name);
    if ((rtn = snd_mixer_open (&this->hidden->mixer_handle, card, setup.mixer_device)) < 0)
    {
        DEBUG_printf (stderr, "snd_mixer_open failed: %s\n", snd_strerror (rtn));
        return -1;
    }

    /* We're ready to rock and roll. :-) */
    return(0);
}
