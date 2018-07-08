/*
 * BW_Midi_Sequencer - MIDI Sequencer for C++
 *
 * Copyright (c) 2015-2018 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "midi_sequencer.hpp"
#include <stdio.h>
#include <memory>
#include <cstring>
#include <cerrno>
#include <iterator>  // std::back_inserter
#include <algorithm> // std::copy
#include <set>
#include <assert.h>

#if defined(_WIN32) && !defined(__WATCOMC__)
#   ifdef _MSC_VER
#       ifdef _WIN64
typedef __int64 ssize_t;
#       else
typedef __int32 ssize_t;
#       endif
#   else
#       ifdef _WIN64
typedef int64_t ssize_t;
#       else
typedef int32_t ssize_t;
#       endif
#   endif
#endif

#ifndef BWMIDI_DISABLE_MUS_SUPPORT
#include "cvt_mus2mid.hpp"
#endif//MUS

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
#include "cvt_xmi2mid.hpp"
#endif//XMI

/**
 * @brief Utility function to read Big-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t readBEint(const void *buffer, size_t nbytes)
{
    uint64_t result = 0;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

    for(size_t n = 0; n < nbytes; ++n)
        result = (result << 8) + data[n];

    return result;
}

/**
 * @brief Utility function to read Little-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t readLEint(const void *buffer, size_t nbytes)
{
    uint64_t result = 0;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

    for(size_t n = 0; n < nbytes; ++n)
        result = result + static_cast<uint64_t>(data[n] << (n * 8));

    return result;
}

/**
 * @brief Secure Standard MIDI Variable-Length numeric value parser with anti-out-of-range protection
 * @param [_inout] ptr Pointer to memory block that contains begin of variable-length value, will be iterated forward
 * @param [_in end Pointer to end of memory block where variable-length value is stored (after end of track)
 * @param [_out] ok Reference to boolean which takes result of variable-length value parsing
 * @return Unsigned integer that conains parsed variable-length value
 */
static inline uint64_t readVarLenEx(const uint8_t **ptr, const uint8_t *end, bool &ok)
{
    uint64_t result = 0;
    ok = false;

    for(;;)
    {
        if(*ptr >= end)
            return 2;
        unsigned char byte = *((*ptr)++);
        result = (result << 7) + (byte & 0x7F);
        if(!(byte & 0x80))
            break;
    }

    ok = true;
    return result;
}

BW_MidiSequencer::MidiEvent::MidiEvent() :
    type(T_UNKNOWN),
    subtype(T_UNKNOWN),
    channel(0),
    isValid(1),
    absPosition(0)
{}

BW_MidiSequencer::MidiTrackRow::MidiTrackRow() :
    time(0.0),
    delay(0),
    absPos(0),
    timeDelay(0.0)
{}

void BW_MidiSequencer::MidiTrackRow::clear()
{
    time = 0.0;
    delay = 0;
    absPos = 0;
    timeDelay = 0.0;
    events.clear();
}

void BW_MidiSequencer::MidiTrackRow::sortEvents(bool *noteStates)
{
    typedef std::vector<MidiEvent> EvtArr;
    EvtArr sysEx;
    EvtArr metas;
    EvtArr noteOffs;
    EvtArr controllers;
    EvtArr anyOther;

    for(size_t i = 0; i < events.size(); i++)
    {
        if(events[i].type == MidiEvent::T_NOTEOFF)
        {
            if(noteOffs.capacity() == 0)
                noteOffs.reserve(events.size());
            noteOffs.push_back(events[i]);
        }
        else if(events[i].type == MidiEvent::T_SYSEX ||
                events[i].type == MidiEvent::T_SYSEX2)
        {
            if(sysEx.capacity() == 0)
                sysEx.reserve(events.size());
            sysEx.push_back(events[i]);
        }
        else if((events[i].type == MidiEvent::T_CTRLCHANGE)
                || (events[i].type == MidiEvent::T_PATCHCHANGE)
                || (events[i].type == MidiEvent::T_WHEEL)
                || (events[i].type == MidiEvent::T_CHANAFTTOUCH))
        {
            if(controllers.capacity() == 0)
                controllers.reserve(events.size());
            controllers.push_back(events[i]);
        }
        else if((events[i].type == MidiEvent::T_SPECIAL)
            && ((events[i].subtype == MidiEvent::ST_MARKER) || (events[i].subtype == MidiEvent::ST_DEVICESWITCH)))
        {
            if(metas.capacity() == 0)
                metas.reserve(events.size());
            metas.push_back(events[i]);
        }
        else
        {
            if(anyOther.capacity() == 0)
                anyOther.reserve(events.size());
            anyOther.push_back(events[i]);
        }
    }

    /*
     * If Note-Off and it's Note-On is on the same row - move this damned note off down!
     */
    if(noteStates)
    {
        std::set<size_t> markAsOn;
        for(size_t i = 0; i < anyOther.size(); i++)
        {
            const MidiEvent e = anyOther[i];
            if(e.type == MidiEvent::T_NOTEON)
            {
                const size_t note_i = (e.channel * 255) + (e.data[0] & 0x7F);
                //Check, was previously note is on or off
                bool wasOn = noteStates[note_i];
                markAsOn.insert(note_i);
                // Detect zero-length notes are following previously pressed note
                int noteOffsOnSameNote = 0;
                for(EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end();)
                {
                    //If note was off, and note-off on same row with note-on - move it down!
                    if(
                        ((*j).channel == e.channel) &&
                        ((*j).data[0] == e.data[0])
                    )
                    {
                        //If note is already off OR more than one note-off on same row and same note
                        if(!wasOn || (noteOffsOnSameNote != 0))
                        {
                            anyOther.push_back(*j);
                            j = noteOffs.erase(j);
                            markAsOn.erase(note_i);
                            continue;
                        }
                        else
                        {
                            //When same row has many note-offs on same row
                            //that means a zero-length note follows previous note
                            //it must be shuted down
                            noteOffsOnSameNote++;
                        }
                    }
                    j++;
                }
            }
        }

        //Mark other notes as released
        for(EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end(); j++)
        {
            size_t note_i = (j->channel * 255) + (j->data[0] & 0x7F);
            noteStates[note_i] = false;
        }

        for(std::set<size_t>::iterator j = markAsOn.begin(); j != markAsOn.end(); j++)
            noteStates[*j] = true;
    }
    /***********************************************************************************/

    events.clear();
    if(!sysEx.empty())
        events.insert(events.end(), sysEx.begin(), sysEx.end());
    if(!noteOffs.empty())
        events.insert(events.end(), noteOffs.begin(), noteOffs.end());
    if(!metas.empty())
        events.insert(events.end(), metas.begin(), metas.end());
    if(!controllers.empty())
        events.insert(events.end(), controllers.begin(), controllers.end());
    if(!anyOther.empty())
        events.insert(events.end(), anyOther.begin(), anyOther.end());
}

BW_MidiSequencer::BW_MidiSequencer() :
    m_interface(NULL),
    m_format(Format_MIDI),
    m_smfFormat(0),
    m_loopEnabled(false),
    m_fullSongTimeLength(0.0),
    m_postSongWaitDelay(1.0),
    m_loopStartTime(-1.0),
    m_loopEndTime(-1.0),
    m_tempoMultiplier(1.0),
    m_atEnd(false),
    m_loopStart(false),
    m_loopEnd(false),
    m_invalidLoop(false),
    m_trackSolo(~(size_t)0)
{}

BW_MidiSequencer::~BW_MidiSequencer()
{}

void BW_MidiSequencer::setInterface(const BW_MidiRtInterface *intrf)
{
    // Interface must NOT be NULL
    assert(intrf);

    //Note ON hook is REQUIRED
    assert(intrf->rt_noteOn);
    //Note OFF hook is REQUIRED
    assert(intrf->rt_noteOff);
    //Note Aftertouch hook is REQUIRED
    assert(intrf->rt_noteAfterTouch);
    //Channel Aftertouch hook is REQUIRED
    assert(intrf->rt_channelAfterTouch);
    //Controller change hook is REQUIRED
    assert(intrf->rt_controllerChange);
    //Patch change hook is REQUIRED
    assert(intrf->rt_patchChange);
    //Pitch bend hook is REQUIRED
    assert(intrf->rt_pitchBend);
    //System Exclusive hook is REQUIRED
    assert(intrf->rt_systemExclusive);

    m_interface = intrf;
}

BW_MidiSequencer::FileFormat BW_MidiSequencer::getFormat()
{
    return m_format;
}

size_t BW_MidiSequencer::getTrackCount() const
{
    return m_trackData.size();
}

bool BW_MidiSequencer::setTrackEnabled(size_t track, bool enable)
{
    size_t trackCount = m_trackData.size();
    if(track >= trackCount)
        return false;
    m_trackDisable[track] = !enable;
    return true;
}

void BW_MidiSequencer::setSoloTrack(size_t track)
{
    m_trackSolo = track;
}

const std::vector<BW_MidiSequencer::CmfInstrument> BW_MidiSequencer::getRawCmfInstruments()
{
    return m_cmfInstruments;
}

const std::string &BW_MidiSequencer::getErrorString()
{
    return m_errorString;
}

bool BW_MidiSequencer::getLoopEnabled()
{
    return m_loopEnabled;
}

void BW_MidiSequencer::setLoopEnabled(bool enabled)
{
    m_loopEnabled = enabled;
}

const std::string &BW_MidiSequencer::getMusicTitle()
{
    return m_musTitle;
}

const std::string &BW_MidiSequencer::getMusicCopyright()
{
    return m_musCopyright;
}

const std::vector<std::string> &BW_MidiSequencer::getTrackTitles()
{
    return m_musTrackTitles;
}

const std::vector<BW_MidiSequencer::MIDI_MarkerEntry> &BW_MidiSequencer::getMarkers()
{
    return m_musMarkers;
}

bool BW_MidiSequencer::positionAtEnd()
{
    return m_atEnd;
}

double BW_MidiSequencer::getTempoMultiplier()
{
    return m_tempoMultiplier;
}

bool BW_MidiSequencer::buildTrackData(const std::vector<std::vector<uint8_t> > &trackData)
{
    m_fullSongTimeLength = 0.0;
    m_loopStartTime = -1.0;
    m_loopEndTime = -1.0;
    m_trackDisable.clear();
    m_trackSolo = ~(size_t)0;
    m_musTitle.clear();
    m_musCopyright.clear();
    m_musTrackTitles.clear();
    m_musMarkers.clear();
    m_trackData.clear();
    const size_t    trackCount = trackData.size();
    m_trackData.resize(trackCount, MidiTrackQueue());
    m_trackDisable.resize(trackCount);

    m_invalidLoop = false;
    bool gotLoopStart = false, gotLoopEnd = false, gotLoopEventInThisRow = false;
    //! Tick position of loop start tag
    uint64_t loopStartTicks = 0;
    //! Tick position of loop end tag
    uint64_t loopEndTicks = 0;
    //! Full length of song in ticks
    uint64_t ticksSongLength = 0;
    //! Cache for error message strign
    char error[150];

    m_currentPosition.track.clear();
    m_currentPosition.track.resize(trackCount);

    //! Caches note on/off states.
    bool noteStates[16 * 255];
    /* This is required to carefully detect zero-length notes           *
     * and avoid a move of "note-off" event over "note-on" while sort.  *
     * Otherwise, after sort those notes will play infinite sound       */

    //Tempo change events
    std::vector<MidiEvent> tempos;

    /*
     * TODO: Make this be safer for memory in case of broken input data
     * which may cause going away of available track data (and then give a crash!)
     *
     * POST: Check this more carefully for possible vulnuabilities are can crash this
     */
    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        uint64_t abs_position = 0;
        int status = 0;
        MidiEvent event;
        bool ok = false;
        const uint8_t *end      = trackData[tk].data() + trackData[tk].size();
        const uint8_t *trackPtr = trackData[tk].data();
        std::memset(noteStates, 0, sizeof(noteStates));

        //Time delay that follows the first event in the track
        {
            MidiTrackRow evtPos;
            if(m_format == Format_RSXX)
                ok = true;
            else
                evtPos.delay = readVarLenEx(&trackPtr, end, ok);
            if(!ok)
            {
                int len = snprintf(error, 150, "buildTrackData: Can't read variable-length value at begin of track %d.\n", (int)tk);
                if((len > 0) && (len < 150))
                    m_parsingErrorsString += std::string(error, (size_t)len);
                return false;
            }

            //HACK: Begin every track with "Reset all controllers" event to avoid controllers state break came from end of song
            for(uint8_t chan = 0; chan < 16; chan++)
            {
                MidiEvent event;
                event.type = MidiEvent::T_CTRLCHANGE;
                event.channel = chan;
                event.data.push_back(121);
                event.data.push_back(0);
                evtPos.events.push_back(event);
            }

            evtPos.absPos = abs_position;
            abs_position += evtPos.delay;
            m_trackData[tk].push_back(evtPos);
        }

        MidiTrackRow evtPos;
        do
        {
            event = parseEvent(&trackPtr, end, status);
            if(!event.isValid)
            {
                int len = snprintf(error, 150, "buildTrackData: Fail to parse event in the track %d.\n", (int)tk);
                if((len > 0) && (len < 150))
                    m_parsingErrorsString += std::string(error, (size_t)len);
                return false;
            }

            evtPos.events.push_back(event);
            if(event.type == MidiEvent::T_SPECIAL)
            {
                if(event.subtype == MidiEvent::ST_TEMPOCHANGE)
                {
                    event.absPosition = abs_position;
                    tempos.push_back(event);
                }
                else if(!m_invalidLoop && (event.subtype == MidiEvent::ST_LOOPSTART))
                {
                    /*
                     * loopStart is invalid when:
                     * - starts together with loopEnd
                     * - appears more than one time in same MIDI file
                     */
                    if(gotLoopStart || gotLoopEventInThisRow)
                        m_invalidLoop = true;
                    else
                    {
                        gotLoopStart = true;
                        loopStartTicks = abs_position;
                    }
                    //In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if(!m_invalidLoop && (event.subtype == MidiEvent::ST_LOOPEND))
                {
                    /*
                     * loopEnd is invalid when:
                     * - starts before loopStart
                     * - starts together with loopStart
                     * - appars more than one time in same MIDI file
                     */
                    if(gotLoopEnd || gotLoopEventInThisRow)
                        m_invalidLoop = true;
                    else
                    {
                        gotLoopEnd = true;
                        loopEndTicks = abs_position;
                    }
                    //In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
            }

            if(event.subtype != MidiEvent::ST_ENDTRACK)//Don't try to read delta after EndOfTrack event!
            {
                evtPos.delay = readVarLenEx(&trackPtr, end, ok);
                if(!ok)
                {
                    /* End of track has been reached! However, there is no EOT event presented */
                    event.type = MidiEvent::T_SPECIAL;
                    event.subtype = MidiEvent::ST_ENDTRACK;
                }
            }

            if((evtPos.delay > 0) || (event.subtype == MidiEvent::ST_ENDTRACK))
            {
                evtPos.absPos = abs_position;
                abs_position += evtPos.delay;
                evtPos.sortEvents(noteStates);
                m_trackData[tk].push_back(evtPos);
                evtPos.clear();
                gotLoopEventInThisRow = false;
            }
        }
        while((trackPtr <= end) && (event.subtype != MidiEvent::ST_ENDTRACK));

        if(ticksSongLength < abs_position)
            ticksSongLength = abs_position;
        //Set the chain of events begin
        if(m_trackData[tk].size() > 0)
            m_currentPosition.track[tk].pos = m_trackData[tk].begin();
    }

    if(gotLoopStart && !gotLoopEnd)
    {
        gotLoopEnd = true;
        loopEndTicks = ticksSongLength;
    }

    //loopStart must be located before loopEnd!
    if(loopStartTicks >= loopEndTicks)
        m_invalidLoop = true;

    /********************************************************************************/
    //Calculate time basing on collected tempo events
    /********************************************************************************/
    for(size_t tk = 0; tk < trackCount; ++tk)
    {
        fraction<uint64_t> currentTempo = m_tempo;
        double  time = 0.0;
        uint64_t abs_position = 0;
        size_t tempo_change_index = 0;
        MidiTrackQueue &track = m_trackData[tk];
        if(track.empty())
            continue;//Empty track is useless!

#ifdef DEBUG_TIME_CALCULATION
        std::fprintf(stdout, "\n============Track %" PRIuPTR "=============\n", tk);
        std::fflush(stdout);
#endif

        MidiTrackRow *posPrev = &(*(track.begin()));//First element
        for(MidiTrackQueue::iterator it = track.begin(); it != track.end(); it++)
        {
#ifdef DEBUG_TIME_CALCULATION
            bool tempoChanged = false;
#endif
            MidiTrackRow &pos = *it;
            if((posPrev != &pos) &&  //Skip first event
               (!tempos.empty()) && //Only when in-track tempo events are available
               (tempo_change_index < tempos.size())
              )
            {
                // If tempo event is going between of current and previous event
                if(tempos[tempo_change_index].absPosition <= pos.absPos)
                {
                    //Stop points: begin point and tempo change points are before end point
                    std::vector<TempoChangePoint> points;
                    fraction<uint64_t> t;
                    TempoChangePoint firstPoint = {posPrev->absPos, currentTempo};
                    points.push_back(firstPoint);

                    //Collect tempo change points between previous and current events
                    do
                    {
                        TempoChangePoint tempoMarker;
                        MidiEvent &tempoPoint = tempos[tempo_change_index];
                        tempoMarker.absPos = tempoPoint.absPosition;
                        tempoMarker.tempo = m_invDeltaTicks * fraction<uint64_t>(readBEint(tempoPoint.data.data(), tempoPoint.data.size()));
                        points.push_back(tempoMarker);
                        tempo_change_index++;
                    }
                    while((tempo_change_index < tempos.size()) &&
                          (tempos[tempo_change_index].absPosition <= pos.absPos));

                    // Re-calculate time delay of previous event
                    time -= posPrev->timeDelay;
                    posPrev->timeDelay = 0.0;

                    for(size_t i = 0, j = 1; j < points.size(); i++, j++)
                    {
                        /* If one or more tempo events are appears between of two events,
                         * calculate delays between each tempo point, begin and end */
                        uint64_t midDelay = 0;
                        //Delay between points
                        midDelay  = points[j].absPos - points[i].absPos;
                        //Time delay between points
                        t = midDelay * currentTempo;
                        posPrev->timeDelay += t.value();

                        //Apply next tempo
                        currentTempo = points[j].tempo;
#ifdef DEBUG_TIME_CALCULATION
                        tempoChanged = true;
#endif
                    }
                    //Then calculate time between last tempo change point and end point
                    TempoChangePoint tailTempo = points.back();
                    uint64_t postDelay = pos.absPos - tailTempo.absPos;
                    t = postDelay * currentTempo;
                    posPrev->timeDelay += t.value();

                    //Store Common time delay
                    posPrev->time = time;
                    time += posPrev->timeDelay;
                }
            }

            fraction<uint64_t> t = pos.delay * currentTempo;
            pos.timeDelay = t.value();
            pos.time = time;
            time += pos.timeDelay;

            //Capture markers after time value calculation
            for(size_t i = 0; i < pos.events.size(); i++)
            {
                MidiEvent &e = pos.events[i];
                if((e.type == MidiEvent::T_SPECIAL) && (e.subtype == MidiEvent::ST_MARKER))
                {
                    MIDI_MarkerEntry marker;
                    marker.label = std::string((char *)e.data.data(), e.data.size());
                    marker.pos_ticks = pos.absPos;
                    marker.pos_time = pos.time;
                    m_musMarkers.push_back(marker);
                }
            }

            //Capture loop points time positions
            if(!m_invalidLoop)
            {
                // Set loop points times
                if(loopStartTicks == pos.absPos)
                    m_loopStartTime = pos.time;
                else if(loopEndTicks == pos.absPos)
                    m_loopEndTime = pos.time;
            }

#ifdef DEBUG_TIME_CALCULATION
            std::fprintf(stdout, "= %10" PRId64 " = %10f%s\n", pos.absPos, pos.time, tempoChanged ? " <----TEMPO CHANGED" : "");
            std::fflush(stdout);
#endif

            abs_position += pos.delay;
            posPrev = &pos;
        }

        if(time > m_fullSongTimeLength)
            m_fullSongTimeLength = time;
    }

    m_fullSongTimeLength += m_postSongWaitDelay;
    //Set begin of the music
    m_trackBeginPosition = m_currentPosition;
    //Initial loop position will begin at begin of track until passing of the loop point
    m_loopBeginPosition  = m_currentPosition;

    /********************************************************************************/
    //Resolve "hell of all times" of too short drum notes:
    //move too short percussion note-offs far far away as possible
    /********************************************************************************/
#if 1 //Use this to record WAVEs for comparison before/after implementing of this
    if(m_format == Format_MIDI)//Percussion fix is needed for MIDI only, not for IMF/RSXX or CMF
    {
        //! Minimal real time in seconds
#define DRUM_NOTE_MIN_TIME  0.03
        //! Minimal ticks count
#define DRUM_NOTE_MIN_TICKS 15
        struct NoteState
        {
            double       delay;
            uint64_t     delayTicks;
            bool         isOn;
            char         ___pad[7];
        } drNotes[255];
        size_t banks[16];

        for(size_t tk = 0; tk < trackCount; ++tk)
        {
            std::memset(drNotes, 0, sizeof(drNotes));
            std::memset(banks, 0, sizeof(banks));
            MidiTrackQueue &track = m_trackData[tk];
            if(track.empty())
                continue;//Empty track is useless!

            for(MidiTrackQueue::iterator it = track.begin(); it != track.end(); it++)
            {
                MidiTrackRow &pos = *it;

                for(ssize_t e = 0; e < (ssize_t)pos.events.size(); e++)
                {
                    MidiEvent *et = &pos.events[(size_t)e];

                    /* Set MSB/LSB bank */
                    if(et->type == MidiEvent::T_CTRLCHANGE)
                    {
                        uint8_t ctrlno = et->data[0];
                        uint8_t value =  et->data[1];
                        switch(ctrlno)
                        {
                        case 0: // Set bank msb (GM bank)
                            banks[et->channel] = (value << 8) | (banks[et->channel] & 0x00FF);
                            break;
                        case 32: // Set bank lsb (XG bank)
                            banks[et->channel] = (banks[et->channel] & 0xFF00) | (value & 0x00FF);
                            break;
                        }
                        continue;
                    }

                    bool percussion = (et->channel == 9) ||
                                      banks[et->channel] == 0x7E00 || //XG SFX1/SFX2 channel (16128 signed decimal)
                                      banks[et->channel] == 0x7F00;   //XG Percussion channel (16256 signed decimal)
                    if(!percussion)
                        continue;

                    if(et->type == MidiEvent::T_NOTEON)
                    {
                        uint8_t     note = et->data[0] & 0x7F;
                        NoteState   &ns = drNotes[note];
                        ns.isOn = true;
                        ns.delay = 0.0;
                        ns.delayTicks = 0;
                    }
                    else if(et->type == MidiEvent::T_NOTEOFF)
                    {
                        uint8_t note = et->data[0] & 0x7F;
                        NoteState &ns = drNotes[note];
                        if(ns.isOn)
                        {
                            ns.isOn = false;
                            if(ns.delayTicks < DRUM_NOTE_MIN_TICKS || ns.delay < DRUM_NOTE_MIN_TIME)//If note is too short
                            {
                                //Move it into next event position if that possible
                                for(MidiTrackQueue::iterator itNext = it;
                                    itNext != track.end();
                                    itNext++)
                                {
                                    MidiTrackRow &posN = *itNext;
                                    if(ns.delayTicks > DRUM_NOTE_MIN_TICKS && ns.delay > DRUM_NOTE_MIN_TIME)
                                    {
                                        //Put note-off into begin of next event list
                                        posN.events.insert(posN.events.begin(), pos.events[(size_t)e]);
                                        //Renive this event from a current row
                                        pos.events.erase(pos.events.begin() + (int)e);
                                        e--;
                                        break;
                                    }
                                    ns.delay += posN.timeDelay;
                                    ns.delayTicks += posN.delay;
                                }
                            }
                            ns.delay = 0.0;
                            ns.delayTicks = 0;
                        }
                    }
                }

                //Append time delays to sustaining notes
                for(size_t no = 0; no < 128; no++)
                {
                    NoteState &ns = drNotes[no];
                    if(ns.isOn)
                    {
                        ns.delay        += pos.timeDelay;
                        ns.delayTicks   += pos.delay;
                    }
                }
            }
        }
#undef DRUM_NOTE_MIN_TIME
#undef DRUM_NOTE_MIN_TICKS
    }
#endif

    return true;
}

bool BW_MidiSequencer::processEvents(bool isSeek)
{
    if(m_currentPosition.track.size() == 0)
        m_atEnd = true;//No MIDI track data to play
    if(m_atEnd)
        return false;//No more events in the queue

    m_loopEnd = false;
    const size_t        TrackCount = m_currentPosition.track.size();
    const Position   RowBeginPosition(m_currentPosition);

#ifdef DEBUG_TIME_CALCULATION
    double maxTime = 0.0;
#endif

    for(size_t tk = 0; tk < TrackCount; ++tk)
    {
        Position::TrackInfo &track = m_currentPosition.track[tk];
        if((track.lastHandledEvent >= 0) && (track.delay <= 0))
        {
            //Check is an end of track has been reached
            if(track.pos == m_trackData[tk].end())
            {
                track.lastHandledEvent = -1;
                break;
            }

            // Handle event
            for(size_t i = 0; i < track.pos->events.size(); i++)
            {
                const MidiEvent &evt = track.pos->events[i];
#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
                if(!CurrentPositionNew.began && (evt.type == MidiEvent::T_NOTEON))
                    CurrentPositionNew.began = true;
#endif
                if(isSeek && (evt.type == MidiEvent::T_NOTEON))
                    continue;
                handleEvent(tk, evt, track.lastHandledEvent);
                if(m_loopEnd)
                    break;//Stop event handling on catching loopEnd event!
            }

#ifdef DEBUG_TIME_CALCULATION
            if(maxTime < track.pos->time)
                maxTime = track.pos->time;
#endif
            // Read next event time (unless the track just ended)
            if(track.lastHandledEvent >= 0)
            {
                track.delay += track.pos->delay;
                track.pos++;
            }
        }
    }

#ifdef DEBUG_TIME_CALCULATION
    std::fprintf(stdout, "                              \r");
    std::fprintf(stdout, "Time: %10f; Audio: %10f\r", maxTime, m_currentPosition.absTimePosition);
    std::fflush(stdout);
#endif

    // Find shortest delay from all track
    uint64_t shortest = 0;
    bool     shortest_no = true;

    for(size_t tk = 0; tk < TrackCount; ++tk)
    {
        Position::TrackInfo &track = m_currentPosition.track[tk];
        if((track.lastHandledEvent >= 0) && (shortest_no || track.delay < shortest))
        {
            shortest = track.delay;
            shortest_no = false;
        }
    }

    //if(shortest > 0) UI.PrintLn("shortest: %ld", shortest);

    // Schedule the next playevent to be processed after that delay
    for(size_t tk = 0; tk < TrackCount; ++tk)
        m_currentPosition.track[tk].delay -= shortest;

    fraction<uint64_t> t = shortest * m_tempo;

#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
    if(CurrentPositionNew.began)
#endif
        m_currentPosition.wait += t.value();

    //if(shortest > 0) UI.PrintLn("Delay %ld (%g)", shortest, (double)t.valuel());
    if(m_loopStart)
    {
        m_loopBeginPosition = RowBeginPosition;
        m_loopStart = false;
    }

    if(shortest_no || m_loopEnd)
    {
        //Loop if song end or loop end point has reached
        m_loopEnd         = false;
        shortest = 0;
        if(!m_loopEnabled)
        {
            m_atEnd = true; //Don't handle events anymore
            m_currentPosition.wait += m_postSongWaitDelay;//One second delay until stop playing
            return true;//We have caugh end here!
        }
        m_currentPosition = m_loopBeginPosition;
    }

    return true;//Has events in queue
}

BW_MidiSequencer::MidiEvent BW_MidiSequencer::parseEvent(const uint8_t **pptr, const uint8_t *end, int &status)
{
    const uint8_t *&ptr = *pptr;
    BW_MidiSequencer::MidiEvent evt;

    if(ptr + 1 > end)
    {
        //When track doesn't ends on the middle of event data, it's must be fine
        evt.type = MidiEvent::T_SPECIAL;
        evt.subtype = MidiEvent::ST_ENDTRACK;
        return evt;
    }

    unsigned char byte = *(ptr++);
    bool ok = false;

    if(byte == MidiEvent::T_SYSEX || byte == MidiEvent::T_SYSEX2)// Ignore SysEx
    {
        uint64_t length = readVarLenEx(pptr, end, ok);
        if(!ok || (ptr + length > end))
        {
            m_parsingErrorsString += "parseEvent: Can't read SysEx event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = MidiEvent::T_SYSEX;
        evt.data.clear();
        evt.data.push_back(byte);
        std::copy(ptr, ptr + length, std::back_inserter(evt.data));
        ptr += (size_t)length;
        return evt;
    }

    if(byte == MidiEvent::T_SPECIAL)
    {
        // Special event FF
        uint8_t  evtype = *(ptr++);
        uint64_t length = readVarLenEx(pptr, end, ok);
        if(!ok || (ptr + length > end))
        {
            m_parsingErrorsString += "parseEvent: Can't read Special event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        std::string data(length ? (const char *)ptr : 0, (size_t)length);
        ptr += (size_t)length;

        evt.type = byte;
        evt.subtype = evtype;
        evt.data.insert(evt.data.begin(), data.begin(), data.end());

#if 0 /* Print all tempo events */
        if(evt.subtype == MidiEvent::ST_TEMPOCHANGE)
        {
            if(hooks.onDebugMessage)
                hooks.onDebugMessage(hooks.onDebugMessage_userData, "Temp Change: %02X%02X%02X", evt.data[0], evt.data[1], evt.data[2]);
        }
#endif

        /* TODO: Store those meta-strings separately and give ability to read them
         * by external functions (to display song title and copyright in the player) */
        if(evt.subtype == MidiEvent::ST_COPYRIGHT)
        {
            if(m_musCopyright.empty())
            {
                m_musCopyright = std::string((const char *)evt.data.data(), evt.data.size());
                if(m_interface->onDebugMessage)
                    m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Music copyright: %s", m_musCopyright.c_str());
            }
            else if(m_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Extra copyright event: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_SQTRKTITLE)
        {
            if(m_musTitle.empty())
            {
                m_musTitle = std::string((const char *)evt.data.data(), evt.data.size());
                if(m_interface->onDebugMessage)
                    m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Music title: %s", m_musTitle.c_str());
            }
            else if(m_interface->onDebugMessage)
            {
                //TODO: Store track titles and associate them with each track and make API to retreive them
                std::string str((const char *)evt.data.data(), evt.data.size());
                m_musTrackTitles.push_back(str);
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Track title: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_INSTRTITLE)
        {
            if(m_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Instrument: %s", str.c_str());
            }
        }
        else if(evt.subtype == MidiEvent::ST_MARKER)
        {
            //To lower
            for(size_t i = 0; i < data.size(); i++)
            {
                if(data[i] <= 'Z' && data[i] >= 'A')
                    data[i] = static_cast<char>(data[i] - ('Z' - 'z'));
            }

            if(data == "loopstart")
            {
                //Return a custom Loop Start event instead of Marker
                evt.subtype = MidiEvent::ST_LOOPSTART;
                evt.data.clear();//Data is not needed
                return evt;
            }

            if(data == "loopend")
            {
                //Return a custom Loop End event instead of Marker
                evt.subtype = MidiEvent::ST_LOOPEND;
                evt.data.clear();//Data is not needed
                return evt;
            }
        }

        if(evtype == MidiEvent::ST_ENDTRACK)
            status = -1;//Finalize track

        return evt;
    }

    // Any normal event (80..EF)
    if(byte < 0x80)
    {
        byte = static_cast<uint8_t>(status | 0x80);
        ptr--;
    }

    //Sys Com Song Select(Song #) [0-127]
    if(byte == MidiEvent::T_SYSCOMSNGSEL)
    {
        if(ptr + 1 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read System Command Song Select event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        return evt;
    }

    //Sys Com Song Position Pntr [LSB, MSB]
    if(byte == MidiEvent::T_SYSCOMSPOSPTR)
    {
        if(ptr + 2 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read System Command Position Pointer event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));
        return evt;
    }

    uint8_t midCh = byte & 0x0F, evType = (byte >> 4) & 0x0F;
    status = byte;
    evt.channel = midCh;
    evt.type = evType;

    switch(evType)
    {
    case MidiEvent::T_NOTEOFF://2 byte length
    case MidiEvent::T_NOTEON:
    case MidiEvent::T_NOTETOUCH:
    case MidiEvent::T_CTRLCHANGE:
    case MidiEvent::T_WHEEL:
        if(ptr + 2 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read regular 2-byte event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }

        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));

        if((evType == MidiEvent::T_NOTEON) && (evt.data[1] == 0))
        {
            evt.type = MidiEvent::T_NOTEOFF; // Note ON with zero velocity is Note OFF!
        } //111'th loopStart controller (RPG Maker and others)
        else if((evType == MidiEvent::T_CTRLCHANGE) && (evt.data[0] == 111))
        {
            //Change event type to custom Loop Start event and clear data
            evt.type = MidiEvent::T_SPECIAL;
            evt.subtype = MidiEvent::ST_LOOPSTART;
            evt.data.clear();
        }

        return evt;
    case MidiEvent::T_PATCHCHANGE://1 byte length
    case MidiEvent::T_CHANAFTTOUCH:
        if(ptr + 1 > end)
        {
            m_parsingErrorsString += "parseEvent: Can't read regular 1-byte event - Unexpected end of track data.\n";
            evt.isValid = 0;
            return evt;
        }
        evt.data.push_back(*(ptr++));
        return evt;
    }

    return evt;
}

void BW_MidiSequencer::handleEvent(size_t track, const BW_MidiSequencer::MidiEvent &evt, int32_t &status)
{
    if(track == 0 && m_smfFormat < 2 && evt.type == MidiEvent::T_SPECIAL &&
       (evt.subtype == MidiEvent::ST_TEMPOCHANGE || evt.subtype == MidiEvent::ST_TIMESIGNATURE))
    {
        /* never reject track 0 timing events on SMF format != 2
           note: multi-track XMI convert to format 2 SMF */
    }
    else
    {
        if(m_trackSolo != ~(size_t)0 && track != m_trackSolo)
            return;
        if(m_trackDisable[track])
            return;
    }

    if(m_interface->onEvent)
    {
        m_interface->onEvent(m_interface->onEvent_userData,
                             evt.type, evt.subtype, evt.channel,
                             evt.data.data(), evt.data.size());
    }

    if(evt.type == MidiEvent::T_SYSEX || evt.type == MidiEvent::T_SYSEX2) // Ignore SysEx
    {
        //std::string data( length?(const char*) &TrackData[track][CurrentPosition.track[track].ptr]:0, length );
        //UI.PrintLn("SysEx %02X: %u bytes", byte, length/*, data.c_str()*/);
#if 0
        std::fputs("SysEx:", stderr);
        for(size_t i = 0; i < evt.data.size(); ++i)
            std::fprintf(stderr, " %02X", evt.data[i]);
        std::fputc('\n', stderr);
#endif
        m_interface->rt_systemExclusive(m_interface->rtUserData, evt.data.data(), evt.data.size());
        return;
    }

    if(evt.type == MidiEvent::T_SPECIAL)
    {
        // Special event FF
        uint8_t  evtype = evt.subtype;
        uint64_t length = (uint64_t)evt.data.size();
        std::string data(length ? (const char *)evt.data.data() : 0, (size_t)length);

        if(evtype == MidiEvent::ST_ENDTRACK)//End Of Track
        {
            status = -1;
            return;
        }

        if(evtype == MidiEvent::ST_TEMPOCHANGE)//Tempo change
        {
            m_tempo = m_invDeltaTicks * fraction<uint64_t>(readBEint(evt.data.data(), evt.data.size()));
            return;
        }

        if(evtype == MidiEvent::ST_MARKER)//Meta event
        {
            //Do nothing! :-P
            return;
        }

        if(evtype == MidiEvent::ST_DEVICESWITCH)
        {
            if(m_interface->onDebugMessage)
                m_interface->onDebugMessage(m_interface->onDebugMessage_userData, "Switching another device: %s", data.c_str());
            if(m_interface->rt_deviceSwitch)
                m_interface->rt_deviceSwitch(m_interface->rtUserData, track, data.c_str(), data.size());
            return;
        }

        //if(evtype >= 1 && evtype <= 6)
        //    UI.PrintLn("Meta %d: %s", evtype, data.c_str());

        //Turn on Loop handling when loop is enabled
        if(m_loopEnabled && !m_invalidLoop)
        {
            if(evtype == MidiEvent::ST_LOOPSTART) // Special non-spec MIDI loop Start point
            {
                m_loopStart = true;
                return;
            }

            if(evtype == MidiEvent::ST_LOOPEND) // Special non-spec MIDI loop End point
            {
                m_loopEnd = true;
                return;
            }
        }

        if(evtype == MidiEvent::ST_RAWOPL) // Special non-spec ADLMIDI special for IMF playback: Direct poke to AdLib
        {
            if(m_interface->rt_rawOPL)
                m_interface->rt_rawOPL(m_interface->rtUserData, static_cast<uint8_t>(data[0]), static_cast<uint8_t>(data[1]));
            return;
        }

        return;
    }

    // Any normal event (80..EF)
    //    if(evt.type < 0x80)
    //    {
    //        byte = static_cast<uint8_t>(CurrentPosition.track[track].status | 0x80);
    //        CurrentPosition.track[track].ptr--;
    //    }

    if(evt.type == MidiEvent::T_SYSCOMSNGSEL ||
       evt.type == MidiEvent::T_SYSCOMSPOSPTR)
        return;

    /*UI.PrintLn("@%X Track %u: %02X %02X",
                CurrentPosition.track[track].ptr-1, (unsigned)track, byte,
                TrackData[track][CurrentPosition.track[track].ptr]);*/
    size_t midCh = evt.channel;//byte & 0x0F, EvType = byte >> 4;
    if(m_interface->rt_currentDevice)
        midCh += m_interface->rt_currentDevice(m_interface->rtUserData, track);
    status = evt.type;

    switch(evt.type)
    {
    case MidiEvent::T_NOTEOFF: // Note off
    {
        uint8_t note = evt.data[0];
        m_interface->rt_noteOff(m_interface->rtUserData, static_cast<uint8_t>(midCh), note);
        break;
    }

    case MidiEvent::T_NOTEON: // Note on
    {
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        m_interface->rt_noteOn(m_interface->rtUserData, static_cast<uint8_t>(midCh), note, vol);
        break;
    }

    case MidiEvent::T_NOTETOUCH: // Note touch
    {
        uint8_t note = evt.data[0];
        uint8_t vol =  evt.data[1];
        m_interface->rt_noteAfterTouch(m_interface->rtUserData, static_cast<uint8_t>(midCh), note, vol);
        break;
    }

    case MidiEvent::T_CTRLCHANGE: // Controller change
    {
        uint8_t ctrlno = evt.data[0];
        uint8_t value =  evt.data[1];
        m_interface->rt_controllerChange(m_interface->rtUserData, static_cast<uint8_t>(midCh), ctrlno, value);
        break;
    }

    case MidiEvent::T_PATCHCHANGE: // Patch change
    {
        m_interface->rt_patchChange(m_interface->rtUserData, static_cast<uint8_t>(midCh), evt.data[0]);
        break;
    }

    case MidiEvent::T_CHANAFTTOUCH: // Channel after-touch
    {
        uint8_t chanat = evt.data[0];
        m_interface->rt_channelAfterTouch(m_interface->rtUserData, static_cast<uint8_t>(midCh), chanat);
        break;
    }

    case MidiEvent::T_WHEEL: // Wheel/pitch bend
    {
        uint8_t a = evt.data[0];
        uint8_t b = evt.data[1];
        m_interface->rt_pitchBend(m_interface->rtUserData, static_cast<uint8_t>(midCh), b, a);
        break;
    }
    }//switch
}

double BW_MidiSequencer::Tick(double s, double granularity)
{
    assert(m_interface);// MIDI output interface must be defined!

    s *= m_tempoMultiplier;
#ifdef ENABLE_BEGIN_SILENCE_SKIPPING
    if(CurrentPositionNew.began)
#endif
        m_currentPosition.wait -= s;
    m_currentPosition.absTimePosition += s;

    int antiFreezeCounter = 10000;//Limit 10000 loops to avoid freezing
    while((m_currentPosition.wait <= granularity * 0.5) && (antiFreezeCounter > 0))
    {
        //std::fprintf(stderr, "wait = %g...\n", CurrentPosition.wait);
        if(!processEvents())
            break;
        if(m_currentPosition.wait <= 0.0)
            antiFreezeCounter--;
    }

    if(antiFreezeCounter <= 0)
        m_currentPosition.wait += 1.0;/* Add extra 1 second when over 10000 events
                                           with zero delay are been detected */

    if(m_currentPosition.wait < 0.0)//Avoid negative delay value!
        return 0.0;

    return m_currentPosition.wait;
}


double BW_MidiSequencer::seek(double seconds, const double granularity)
{
    if(seconds < 0.0)
        return 0.0;//Seeking negative position is forbidden! :-P
    const double granualityHalf = granularity * 0.5,
                 s = seconds;//m_setup.delay < m_setup.maxdelay ? m_setup.delay : m_setup.maxdelay;

    /* Attempt to go away out of song end must rewind position to begin */
    if(seconds > m_fullSongTimeLength)
    {
        rewind();
        return 0.0;
    }

    bool loopFlagState = m_loopEnabled;
    // Turn loop pooints off because it causes wrong position rememberin on a quick seek
    m_loopEnabled = false;

    /*
     * Seeking search is similar to regular ticking, except of next things:
     * - We don't processsing arpeggio and vibrato
     * - To keep correctness of the state after seek, begin every search from begin
     * - All sustaining notes must be killed
     * - Ignore Note-On events
     */
    rewind();

    /*
     * Set "loop Start" to false to prevent overwrite of loopStart position with
     * seek destinition position
     *
     * TODO: Detect & set loopStart position on load time to don't break loop while seeking
     */
    m_loopStart   = false;

    while((m_currentPosition.absTimePosition < seconds) &&
          (m_currentPosition.absTimePosition < m_fullSongTimeLength))
    {
        m_currentPosition.wait -= s;
        m_currentPosition.absTimePosition += s;
        int antiFreezeCounter = 10000;//Limit 10000 loops to avoid freezing
        double dstWait = m_currentPosition.wait + granualityHalf;
        while((m_currentPosition.wait <= granualityHalf)/*&& (antiFreezeCounter > 0)*/)
        {
            //std::fprintf(stderr, "wait = %g...\n", CurrentPosition.wait);
            if(!processEvents(true))
                break;
            //Avoid freeze because of no waiting increasing in more than 10000 cycles
            if(m_currentPosition.wait <= dstWait)
                antiFreezeCounter--;
            else
            {
                dstWait = m_currentPosition.wait + granualityHalf;
                antiFreezeCounter = 10000;
            }
        }
        if(antiFreezeCounter <= 0)
            m_currentPosition.wait += 1.0;/* Add extra 1 second when over 10000 events
                                               with zero delay are been detected */
    }

    if(m_currentPosition.wait < 0.0)
        m_currentPosition.wait = 0.0;

    m_loopEnabled = loopFlagState;
    return m_currentPosition.wait;
}

double BW_MidiSequencer::tell()
{
    return m_currentPosition.absTimePosition;
}

double BW_MidiSequencer::timeLength()
{
    return m_fullSongTimeLength;
}

double BW_MidiSequencer::getLoopStart()
{
    return m_loopStartTime;
}

double BW_MidiSequencer::getLoopEnd()
{
    return m_loopEndTime;
}

void BW_MidiSequencer::rewind()
{
    m_currentPosition   = m_trackBeginPosition;
    m_atEnd            = false;
    m_loopStart        = true;
    m_loopEnd          = false;
}

void BW_MidiSequencer::setTempo(double tempo)
{
    m_tempoMultiplier = tempo;
}

bool BW_MidiSequencer::loadMIDI(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    if(!loadMIDI(file))
        return false;
    return true;
}

bool BW_MidiSequencer::loadMIDI(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, size);
    return loadMIDI(file);
}

template<class T>
class BufferGuard
{
    T *m_ptr;
public:
    BufferGuard() : m_ptr(NULL)
    {}

    ~BufferGuard()
    {
        set();
    }

    void set(T *p = NULL)
    {
        if(m_ptr)
            free(m_ptr);
        m_ptr = p;
    }
};

bool BW_MidiSequencer::loadMIDI(FileAndMemReader &fr)
{
    size_t  fsize;
    BW_MidiSequencer_UNUSED(fsize);
    std::vector<std::vector<uint8_t> > rawTrackData;
    //! Temp buffer for conversion
    BufferGuard<uint8_t> cvt_buf;
    m_parsingErrorsString.clear();

    assert(m_interface);// MIDI output interface must be defined!

    if(!fr.isValid())
    {
        m_errorString = "Invalid data stream!\n";
#ifndef _WIN32
        m_errorString += std::strerror(errno);
#endif
        return false;
    }

    m_atEnd            = false;
    m_loopStart        = true;
    m_invalidLoop      = false;

    m_format = Format_MIDI;

    bool is_GMF = false; // GMD/MUS files (ScummVM)
    bool is_IMF = false; // IMF
    bool is_CMF = false; // Creative Music format (CMF/CTMF)
    bool is_RSXX = false; // RSXX, such as Cartooners

    const size_t headerSize = 4 + 4 + 2 + 2 + 2; // 14
    char headerBuf[headerSize] = "";
    size_t DeltaTicks = 192, TrackCount = 1;
    unsigned Fmt = 0;

riffskip:
    fsize = fr.read(headerBuf, 1, headerSize);
    if(fsize < headerSize)
    {
        m_errorString = "Unexpected end of file at header!\n";
        return false;
    }

    if(std::memcmp(headerBuf, "RIFF", 4) == 0)
    {
        fr.seek(6l, FileAndMemReader::CUR);
        goto riffskip;
    }

    if(std::memcmp(headerBuf, "GMF\x1", 4) == 0)
    {
        // GMD/MUS files (ScummVM)
        fr.seek(7 - static_cast<long>(headerSize), FileAndMemReader::CUR);
        is_GMF = true;
    }
#ifndef BWMIDI_DISABLE_MUS_SUPPORT
    else if(std::memcmp(headerBuf, "MUS\x1A", 4) == 0)
    {
        // MUS/DMX files (Doom)
        size_t mus_len = fr.fileSize();
        fr.seek(0, FileAndMemReader::SET);
        uint8_t *mus = (uint8_t *)malloc(mus_len);
        if(!mus)
        {
            m_errorString = "Out of memory!";
            return false;
        }
        fsize = fr.read(mus, 1, mus_len);
        if(fsize < mus_len)
        {
            fr.close();
            m_errorString = "Failed to read MUS file data!\n";
            return false;
        }

        //Close source stream
        fr.close();

        uint8_t *mid = NULL;
        uint32_t mid_len = 0;
        int m2mret = Convert_mus2midi(mus, static_cast<uint32_t>(mus_len),
                                      &mid, &mid_len, 0);
        if(mus)
            free(mus);
        if(m2mret < 0)
        {
            m_errorString = "Invalid MUS/DMX data format!";
            return false;
        }
        cvt_buf.set(mid);
        //Open converted MIDI file
        fr.openData(mid, static_cast<size_t>(mid_len));
        //Re-Read header again!
        goto riffskip;
    }
#endif //BWMIDI_DISABLE_MUS_SUPPORT

#ifndef BWMIDI_DISABLE_XMI_SUPPORT
    else if(std::memcmp(headerBuf, "FORM", 4) == 0)
    {
        if(std::memcmp(headerBuf + 8, "XDIR", 4) != 0)
        {
            fr.close();
            m_errorString = fr.fileName() + ": Invalid format\n";
            return false;
        }

        size_t mus_len = fr.fileSize();
        fr.seek(0, FileAndMemReader::SET);

        uint8_t *mus = (uint8_t*)malloc(mus_len);
        if(!mus)
        {
            m_errorString = "Out of memory!";
            return false;
        }
        fsize = fr.read(mus, 1, mus_len);
        if(fsize < mus_len)
        {
            fr.close();
            m_errorString = "Failed to read XMI file data!\n";
            return false;
        }

        //Close source stream
        fr.close();

        uint8_t *mid = NULL;
        uint32_t mid_len = 0;
        int m2mret = Convert_xmi2midi(mus, static_cast<uint32_t>(mus_len),
                                      &mid, &mid_len, XMIDI_CONVERT_NOCONVERSION);
        if(mus) free(mus);
        if(m2mret < 0)
        {
            m_errorString = "Invalid XMI data format!";
            return false;
        }
        cvt_buf.set(mid);
        //Open converted MIDI file
        fr.openData(mid, static_cast<size_t>(mid_len));
        //Re-Read header again!
        goto riffskip;
    }
#endif //BWMIDI_DISABLE_XMI_SUPPORT

    else if(std::memcmp(headerBuf, "CTMF", 4) == 0)
    {
        // Creative Music Format (CMF).
        // When playing CTMF files, use the following commandline:
        // adlmidi song8.ctmf -p -v 1 1 0
        // i.e. enable percussion mode, deeper vibrato, and use only 1 card.
        is_CMF = true;
        m_format = Format_CMF;
        //unsigned version   = ReadLEint(HeaderBuf+4, 2);
        uint64_t ins_start = readLEint(headerBuf + 6, 2);
        uint64_t mus_start = readLEint(headerBuf + 8, 2);
        //unsigned deltas    = ReadLEint(HeaderBuf+10, 2);
        uint64_t ticks     = readLEint(headerBuf + 12, 2);
        // Read title, author, remarks start offsets in file
        fsize = fr.read(headerBuf, 1, 6);
        if(fsize < 6)
        {
            fr.close();
            m_errorString = "Unexpected file ending on attempt to read CTMF header!";
            return false;
        }

        //unsigned long notes_starts[3] = {ReadLEint(HeaderBuf+0,2),ReadLEint(HeaderBuf+0,4),ReadLEint(HeaderBuf+0,6)};
        fr.seek(16, FileAndMemReader::CUR); // Skip the channels-in-use table
        fsize = fr.read(headerBuf, 1, 4);
        if(fsize < 4)
        {
            fr.close();
            m_errorString = "Unexpected file ending on attempt to read CMF instruments block header!";
            return false;
        }

        uint64_t ins_count =  readLEint(headerBuf + 0, 2); //, basictempo = ReadLEint(HeaderBuf+2, 2);
        fr.seek(static_cast<long>(ins_start), FileAndMemReader::SET);

        m_cmfInstruments.reserve(static_cast<size_t>(ins_count));
        for(uint64_t i = 0; i < ins_count; ++i)
        {
            CmfInstrument inst;
            fsize = fr.read(inst.data, 1, 16);
            if(fsize < 16)
            {
                fr.close();
                m_errorString = "Unexpected file ending on attempt to read CMF instruments raw data!";
                return false;
            }
            m_cmfInstruments.push_back(inst);
        }

        fr.seeku(mus_start, FileAndMemReader::SET);
        TrackCount = 1;
        DeltaTicks = (size_t)ticks;
    }
    else
    {
        // Try to identify RSXX format
        if(headerBuf[0] == 0x7D)
        {
            fr.seek(0x6D, FileAndMemReader::SET);
            fr.read(headerBuf, 1, 6);
            if(std::memcmp(headerBuf, "rsxx}u", 6) == 0)
            {
                is_RSXX = true;
                m_format = Format_RSXX;
                fr.seek(0x7D, FileAndMemReader::SET);
                TrackCount = 1;
                DeltaTicks = 60;
            }
        }

        // Try parsing as an IMF file
        if(!is_RSXX)
        {
            do
            {
                uint8_t raw[4];
                size_t end = static_cast<size_t>(headerBuf[0]) + 256 * static_cast<size_t>(headerBuf[1]);

                if(!end || (end & 3))
                    break;

                size_t backup_pos = fr.tell();
                int64_t sum1 = 0, sum2 = 0;
                fr.seek(2, FileAndMemReader::SET);

                for(unsigned n = 0; n < 42; ++n)
                {
                    if(fr.read(raw, 1, 4) != 4)
                        break;
                    int64_t value1 = raw[0];
                    value1 += raw[1] << 8;
                    sum1 += value1;
                    int64_t value2 = raw[2];
                    value2 += raw[3] << 8;
                    sum2 += value2;
                }

                fr.seek(static_cast<long>(backup_pos), FileAndMemReader::SET);

                if(sum1 > sum2)
                {
                    is_IMF = true;
                    m_format = Format_IMF;
                    DeltaTicks = 1;
                }
            } while(false);
        }

        if(!is_IMF && !is_RSXX)
        {
            if(std::memcmp(headerBuf, "MThd\0\0\0\6", 8) != 0)
            {
                fr.close();
                m_errorString = fr.fileName() + ": Invalid format, Header signature is unknown!\n";
                return false;
            }

            Fmt = (unsigned)readBEint(headerBuf + 8,  2);
            TrackCount = (size_t)readBEint(headerBuf + 10, 2);
            DeltaTicks = (size_t)readBEint(headerBuf + 12, 2);

            if(Fmt > 2)
                Fmt = 1;
        }
    }

    rawTrackData.clear();
    rawTrackData.resize(TrackCount, std::vector<uint8_t>());
    m_invDeltaTicks = fraction<uint64_t>(1, 1000000l * static_cast<uint64_t>(DeltaTicks));
    if(is_CMF || is_RSXX)
        m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(DeltaTicks));
    else
        m_tempo         = fraction<uint64_t>(1,            static_cast<uint64_t>(DeltaTicks) * 2);
    static const unsigned char EndTag[4] = {0xFF, 0x2F, 0x00, 0x00};
    size_t totalGotten = 0;

    for(size_t tk = 0; tk < TrackCount; ++tk)
    {
        // Read track header
        size_t trackLength;

        if(is_IMF)
        {
            //std::fprintf(stderr, "Reading IMF file...\n");
            size_t end = static_cast<size_t>(headerBuf[0]) + 256 * static_cast<size_t>(headerBuf[1]);
            unsigned IMF_tempo = 1428;
            static const unsigned char imf_tempo[] = {0x0,//Zero delay!
                                                      MidiEvent::T_SPECIAL, MidiEvent::ST_TEMPOCHANGE, 0x4,
                                                      static_cast<uint8_t>(IMF_tempo >> 24),
                                                      static_cast<uint8_t>(IMF_tempo >> 16),
                                                      static_cast<uint8_t>(IMF_tempo >> 8),
                                                      static_cast<uint8_t>(IMF_tempo)
                                                     };
            rawTrackData[tk].insert(rawTrackData[tk].end(), imf_tempo, imf_tempo + sizeof(imf_tempo));
            rawTrackData[tk].push_back(0x00);
            fr.seek(2, FileAndMemReader::SET);

            while(fr.tell() < end && !fr.eof())
            {
                uint8_t special_event_buf[5];
                uint8_t raw[4];
                special_event_buf[0] = MidiEvent::T_SPECIAL;
                special_event_buf[1] = MidiEvent::ST_RAWOPL;
                special_event_buf[2] = 0x02;
                if(fr.read(raw, 1, 4) != 4)
                    break;
                special_event_buf[3] = raw[0]; // port index
                special_event_buf[4] = raw[1]; // port value
                uint32_t delay = static_cast<uint32_t>(raw[2]);
                delay += 256 * static_cast<uint32_t>(raw[3]);
                totalGotten += 4;
                //if(special_event_buf[3] <= 8) continue;
                //fprintf(stderr, "Put %02X <- %02X, plus %04X delay\n", special_event_buf[3],special_event_buf[4], delay);
                rawTrackData[tk].insert(rawTrackData[tk].end(), special_event_buf, special_event_buf + 5);
                //if(delay>>21) TrackData[tk].push_back( 0x80 | ((delay>>21) & 0x7F ) );
                if(delay >> 14)
                    rawTrackData[tk].push_back(static_cast<uint8_t>(0x80 | ((delay >> 14) & 0x7F)));
                if(delay >> 7)
                    rawTrackData[tk].push_back(static_cast<uint8_t>(0x80 | ((delay >> 7) & 0x7F)));
                rawTrackData[tk].push_back(static_cast<uint8_t>(((delay >> 0) & 0x7F)));
            }

            rawTrackData[tk].insert(rawTrackData[tk].end(), EndTag + 0, EndTag + 4);
        }
        else
        {
            // Take the rest of the file
            if(is_GMF || is_CMF || is_RSXX)
            {
                size_t pos = fr.tell();
                fr.seek(0, FileAndMemReader::END);
                trackLength = fr.tell() - pos;
                fr.seek(static_cast<long>(pos), FileAndMemReader::SET);
            }
            else
            {
                fsize = fr.read(headerBuf, 1, 8);
                if((fsize < 8) || (std::memcmp(headerBuf, "MTrk", 4) != 0))
                {
                    fr.close();
                    m_errorString = fr.fileName() + ": Invalid format, MTrk signature is not found!\n";
                    return false;
                }
                trackLength = (size_t)readBEint(headerBuf + 4, 4);
            }

            // Read track data
            rawTrackData[tk].resize(trackLength);
            fsize = fr.read(&rawTrackData[tk][0], 1, trackLength);
            if(fsize < trackLength)
            {
                fr.close();
                m_errorString = fr.fileName() + ": Unexpected file ending while getting raw track data!\n";
                return false;
            }
            totalGotten += fsize;

            if(is_GMF/*|| is_MUS*/) // Note: CMF does include the track end tag.
                rawTrackData[tk].insert(rawTrackData[tk].end(), EndTag + 0, EndTag + 4);
            if(is_RSXX)//Finalize raw track data with a zero
                rawTrackData[tk].push_back(0);
        }
    }

    for(size_t tk = 0; tk < TrackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if(totalGotten == 0)
    {
        m_errorString = fr.fileName() + ": Empty track data";
        return false;
    }

    // Build new MIDI events table
    if(!buildTrackData(rawTrackData))
    {
        m_errorString = fr.fileName() + ": MIDI data parsing error has occouped!\n" + m_parsingErrorsString;
        return false;
    }

    m_smfFormat = Fmt;

    return true;
}
