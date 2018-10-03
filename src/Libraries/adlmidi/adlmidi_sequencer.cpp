/*
 * libADLMIDI is a free MIDI to WAV conversion library with OPL3 emulation
 *
 * Original ADLMIDI code: Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * ADLMIDI Library API:   Copyright (c) 2015-2018 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the ADLMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/adlmidi.html
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ADLMIDI_DISABLE_MIDI_SEQUENCER

// Rename class to avoid ABI collisions
#define BW_MidiSequencer AdlMidiSequencer
// Inlucde MIDI sequencer class implementation
#include "midi_sequencer_impl.hpp"

#include "adlmidi_private.hpp"

/****************************************************
 *           Real-Time MIDI calls proxies           *
 ****************************************************/

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_NoteOn(channel, note, velocity);
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_NoteOff(channel, note);
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_NoteAfterTouch(channel, note, atVal);
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_ChannelAfterTouch(channel, atVal);
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_Controller(channel, type, value);
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_PatchChange(channel, patch);
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_PitchBend(channel, msb, lsb);
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_SysEx(msg, size);
}


/* NonStandard calls */
static void rtRawOPL(void *userdata, uint8_t reg, uint8_t value)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    return context->realTime_rawOPL(reg, value);
}

static void rtDeviceSwitch(void *userdata, size_t track, const char *data, size_t length)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    context->realTime_deviceSwitch(track, data, length);
}

static size_t rtCurrentDevice(void *userdata, size_t track)
{
    MIDIplay *context = reinterpret_cast<MIDIplay *>(userdata);
    return context->realTime_currentDevice(track);
}
/* NonStandard calls End */


void MIDIplay::initSequencerInterface()
{
    std::memset(&m_sequencerInterface, 0, sizeof(BW_MidiRtInterface));

    m_sequencerInterface.onDebugMessage             = hooks.onDebugMessage;
    m_sequencerInterface.onDebugMessage_userData    = hooks.onDebugMessage_userData;

    /* MIDI Real-Time calls */
    m_sequencerInterface.rtUserData = this;
    m_sequencerInterface.rt_noteOn  = rtNoteOn;
    m_sequencerInterface.rt_noteOff = rtNoteOff;
    m_sequencerInterface.rt_noteAfterTouch = rtNoteAfterTouch;
    m_sequencerInterface.rt_channelAfterTouch = rtChannelAfterTouch;
    m_sequencerInterface.rt_controllerChange = rtControllerChange;
    m_sequencerInterface.rt_patchChange = rtPatchChange;
    m_sequencerInterface.rt_pitchBend = rtPitchBend;
    m_sequencerInterface.rt_systemExclusive = rtSysEx;

    /* NonStandard calls */
    m_sequencerInterface.rt_rawOPL = rtRawOPL;
    m_sequencerInterface.rt_deviceSwitch = rtDeviceSwitch;
    m_sequencerInterface.rt_currentDevice = rtCurrentDevice;
    /* NonStandard calls End */

    m_sequencer.setInterface(&m_sequencerInterface);
}

double MIDIplay::Tick(double s, double granularity)
{
    double ret = m_sequencer.Tick(s, granularity);

    s *= m_sequencer.getTempoMultiplier();
    for(uint16_t c = 0; c < m_synth.m_numChannels; ++c)
        m_chipChannels[c].addAge(static_cast<int64_t>(s * 1000.0));

    updateVibrato(s);
    updateArpeggio(s);
#if !defined(ADLMIDI_AUDIO_TICK_HANDLER)
    updateGlide(s);
#endif

    return ret;
}

#endif /* ADLMIDI_DISABLE_MIDI_SEQUENCER */
