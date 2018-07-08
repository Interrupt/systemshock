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

#include "adlmidi_private.hpp"
#include "wopl/wopl_file.h"

bool MIDIplay::LoadBank(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    return LoadBank(file);
}

bool MIDIplay::LoadBank(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, size);
    return LoadBank(file);
}

template <class WOPLI>
static void cvt_generic_to_FMIns(adlinsdata2 &ins, const WOPLI &in)
{
    ins.voice2_fine_tune = 0.0;
    int8_t voice2_fine_tune = in.second_voice_detune;
    if(voice2_fine_tune != 0)
    {
        if(voice2_fine_tune == 1)
            ins.voice2_fine_tune = 0.000025;
        else if(voice2_fine_tune == -1)
            ins.voice2_fine_tune = -0.000025;
        else
            ins.voice2_fine_tune = voice2_fine_tune * (15.625 / 1000.0);
    }

    ins.tone = in.percussion_key_number;
    ins.flags = (in.inst_flags & WOPL_Ins_4op) && (in.inst_flags & WOPL_Ins_Pseudo4op) ? adlinsdata::Flag_Pseudo4op : 0;
    ins.flags|= (in.inst_flags & WOPL_Ins_4op) && ((in.inst_flags & WOPL_Ins_Pseudo4op) == 0) ? adlinsdata::Flag_Real4op : 0;
    ins.flags|= (in.inst_flags & WOPL_Ins_IsBlank) ? adlinsdata::Flag_NoSound : 0;

    bool fourOps = (in.inst_flags & WOPL_Ins_4op) || (in.inst_flags & WOPL_Ins_Pseudo4op);
    for(size_t op = 0, slt = 0; op < static_cast<size_t>(fourOps ? 4 : 2); op++, slt++)
    {
        ins.adl[slt].carrier_E862 =
            ((static_cast<uint32_t>(in.operators[op].waveform_E0) << 24) & 0xFF000000) //WaveForm
            | ((static_cast<uint32_t>(in.operators[op].susrel_80) << 16) & 0x00FF0000) //SusRel
            | ((static_cast<uint32_t>(in.operators[op].atdec_60) << 8) & 0x0000FF00)   //AtDec
            | ((static_cast<uint32_t>(in.operators[op].avekf_20) << 0) & 0x000000FF);  //AVEKM
        ins.adl[slt].carrier_40 = in.operators[op].ksl_l_40;//KSLL

        op++;
        ins.adl[slt].modulator_E862 =
            ((static_cast<uint32_t>(in.operators[op].waveform_E0) << 24) & 0xFF000000) //WaveForm
            | ((static_cast<uint32_t>(in.operators[op].susrel_80) << 16) & 0x00FF0000) //SusRel
            | ((static_cast<uint32_t>(in.operators[op].atdec_60) << 8) & 0x0000FF00)   //AtDec
            | ((static_cast<uint32_t>(in.operators[op].avekf_20) << 0) & 0x000000FF);  //AVEKM
        ins.adl[slt].modulator_40 = in.operators[op].ksl_l_40;//KSLL
    }

    ins.adl[0].finetune = static_cast<int8_t>(in.note_offset1);
    ins.adl[0].feedconn = in.fb_conn1_C0;
    if(!fourOps)
        ins.adl[1] = ins.adl[0];
    else
    {
        ins.adl[1].finetune = static_cast<int8_t>(in.note_offset2);
        ins.adl[1].feedconn = in.fb_conn2_C0;
    }

    ins.ms_sound_kon  = in.delay_on_ms;
    ins.ms_sound_koff = in.delay_off_ms;
}

template <class WOPLI>
static void cvt_FMIns_to_generic(WOPLI &ins, const adlinsdata2 &in)
{
    ins.second_voice_detune = 0;
    double voice2_fine_tune = in.voice2_fine_tune;
    if(voice2_fine_tune != 0)
    {
        if(voice2_fine_tune > 0 && voice2_fine_tune <= 0.000025)
            ins.second_voice_detune = 1;
        else if(voice2_fine_tune < 0 && voice2_fine_tune >= -0.000025)
            ins.second_voice_detune = -1;
        else
        {
            long value = static_cast<long>(round(voice2_fine_tune * (1000.0 / 15.625)));
            value = (value < -128) ? -128 : value;
            value = (value > +127) ? +127 : value;
            ins.second_voice_detune = static_cast<int8_t>(value);
        }
    }

    ins.percussion_key_number = in.tone;
    bool fourOps = (in.flags & adlinsdata::Flag_Pseudo4op) || in.adl[0] != in.adl[1];
    ins.inst_flags = fourOps ? WOPL_Ins_4op : 0;
    ins.inst_flags|= (in.flags & adlinsdata::Flag_Pseudo4op) ? WOPL_Ins_Pseudo4op : 0;
    ins.inst_flags|= (in.flags & adlinsdata::Flag_NoSound) ? WOPL_Ins_IsBlank : 0;

    for(size_t op = 0, slt = 0; op < static_cast<size_t>(fourOps ? 4 : 2); op++, slt++)
    {
        ins.operators[op].waveform_E0 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 24);
        ins.operators[op].susrel_80 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 16);
        ins.operators[op].atdec_60 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 8);
        ins.operators[op].avekf_20 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 0);
        ins.operators[op].ksl_l_40 = in.adl[slt].carrier_40;

        op++;
        ins.operators[op].waveform_E0 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 24);
        ins.operators[op].susrel_80 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 16);
        ins.operators[op].atdec_60 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 8);
        ins.operators[op].avekf_20 = static_cast<uint8_t>(in.adl[slt].carrier_E862 >> 0);
        ins.operators[op].ksl_l_40 = in.adl[slt].carrier_40;
    }

    ins.note_offset1 = in.adl[0].finetune;
    ins.fb_conn1_C0 = in.adl[0].feedconn;
    if(!fourOps)
    {
        ins.operators[2] = ins.operators[0];
        ins.operators[3] = ins.operators[1];
    }
    else
    {
        ins.note_offset2 = in.adl[1].finetune;
        ins.fb_conn2_C0 = in.adl[1].feedconn;
    }

    ins.delay_on_ms = in.ms_sound_kon;
    ins.delay_off_ms = in.ms_sound_koff;
}

void cvt_ADLI_to_FMIns(adlinsdata2 &ins, const ADL_Instrument &in)
{
    return cvt_generic_to_FMIns(ins, in);
}

void cvt_FMIns_to_ADLI(ADL_Instrument &ins, const adlinsdata2 &in)
{
    cvt_FMIns_to_generic(ins, in);
}

bool MIDIplay::LoadBank(FileAndMemReader &fr)
{
    int err = 0;
    WOPLFile *wopl = NULL;
    char *raw_file_data = NULL;
    size_t  fsize;
    if(!fr.isValid())
    {
        errorStringOut = "Custom bank: Invalid data stream!";
        return false;
    }

    // Read complete bank file into the memory
    fsize = fr.fileSize();
    fr.seek(0, FileAndMemReader::SET);
    // Allocate necessary memory block
    raw_file_data = (char*)malloc(fsize);
    if(!raw_file_data)
    {
        errorStringOut = "Custom bank: Out of memory before of read!";
        return false;
    }
    fr.read(raw_file_data, 1, fsize);

    // Parse bank file from the memory
    wopl = WOPL_LoadBankFromMem((void*)raw_file_data, fsize, &err);
    //Free the buffer no more needed
    free(raw_file_data);

    // Check for any erros
    if(!wopl)
    {
        switch(err)
        {
        case WOPL_ERR_BAD_MAGIC:
            errorStringOut = "Custom bank: Invalid magic!";
            return false;
        case WOPL_ERR_UNEXPECTED_ENDING:
            errorStringOut = "Custom bank: Unexpected ending!";
            return false;
        case WOPL_ERR_INVALID_BANKS_COUNT:
            errorStringOut = "Custom bank: Invalid banks count!";
            return false;
        case WOPL_ERR_NEWER_VERSION:
            errorStringOut = "Custom bank: Version is newer than supported by this library!";
            return false;
        case WOPL_ERR_OUT_OF_MEMORY:
            errorStringOut = "Custom bank: Out of memory!";
            return false;
        default:
            errorStringOut = "Custom bank: Unknown error!";
            return false;
        }
    }

    m_synth.m_insBankSetup.adLibPercussions = false;
    m_synth.m_insBankSetup.scaleModulators = false;
    m_synth.m_insBankSetup.deepTremolo = (wopl->opl_flags & WOPL_FLAG_DEEP_TREMOLO) != 0;
    m_synth.m_insBankSetup.deepVibrato = (wopl->opl_flags & WOPL_FLAG_DEEP_VIBRATO) != 0;
    m_synth.m_insBankSetup.volumeModel = wopl->volume_model;
    m_setup.deepTremoloMode = -1;
    m_setup.deepVibratoMode = -1;
    m_setup.volumeScaleModel = ADLMIDI_VolumeModel_AUTO;

    m_synth.setEmbeddedBank(m_setup.bankId);

    uint16_t slots_counts[2] = {wopl->banks_count_melodic, wopl->banks_count_percussion};
    WOPLBank *slots_src_ins[2] = { wopl->banks_melodic, wopl->banks_percussive };

    for(size_t ss = 0; ss < 2; ss++)
    {
        for(size_t i = 0; i < slots_counts[ss]; i++)
        {
            size_t bankno = (slots_src_ins[ss][i].bank_midi_msb * 256) +
                            (slots_src_ins[ss][i].bank_midi_lsb) +
                            (ss ? size_t(OPL3::PercussionTag) : 0);
            OPL3::Bank &bank = m_synth.m_insBanks[bankno];
            for(int j = 0; j < 128; j++)
            {
                adlinsdata2 &ins = bank.ins[j];
                std::memset(&ins, 0, sizeof(adlinsdata2));
                WOPLInstrument &inIns = slots_src_ins[ss][i].ins[j];
                cvt_generic_to_FMIns(ins, inIns);
            }
        }
    }

    m_synth.m_embeddedBank = OPL3::CustomBankTag; // Use dynamic banks!
    //Percussion offset is count of instruments multipled to count of melodic banks
    applySetup();

    WOPL_Free(wopl);

    return true;
}

#ifndef ADLMIDI_DISABLE_MIDI_SEQUENCER

bool MIDIplay::LoadMIDI_pre()
{
#ifdef DISABLE_EMBEDDED_BANKS
    if((m_synth.m_embeddedBank != OPL3::CustomBankTag) || m_synth.m_insBanks.empty())
    {
        errorStringOut = "Bank is not set! Please load any instruments bank by using of adl_openBankFile() or adl_openBankData() functions!";
        return false;
    }
#endif
    /**** Set all properties BEFORE starting of actial file reading! ****/
    resetMIDI();
    applySetup();

    return true;
}

bool MIDIplay::LoadMIDI_post()
{
    MidiSequencer::FileFormat format = m_sequencer.getFormat();
    if(format == MidiSequencer::Format_CMF)
    {
        const std::vector<MidiSequencer::CmfInstrument> &instruments = m_sequencer.getRawCmfInstruments();
        m_synth.m_insBanks.clear();//Clean up old banks

        uint16_t ins_count = static_cast<uint16_t>(instruments.size());
        for(uint16_t i = 0; i < ins_count; ++i)
        {
            const uint8_t *InsData = instruments[i].data;
            size_t bank = i / 256;
            bank = ((bank & 127) + ((bank >> 7) << 8));
            if(bank > 127 + (127 << 8))
                break;
            bank += (i % 256 < 128) ? 0 : size_t(OPL3::PercussionTag);

            /*std::printf("Ins %3u: %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\n",
                        i, InsData[0],InsData[1],InsData[2],InsData[3], InsData[4],InsData[5],InsData[6],InsData[7],
                           InsData[8],InsData[9],InsData[10],InsData[11], InsData[12],InsData[13],InsData[14],InsData[15]);*/
            adlinsdata2 &adlins = m_synth.m_insBanks[bank].ins[i % 128];
            adldata    adl;
            adl.modulator_E862 =
                ((static_cast<uint32_t>(InsData[8] & 0x07) << 24) & 0xFF000000) //WaveForm
                | ((static_cast<uint32_t>(InsData[6]) << 16) & 0x00FF0000) //Sustain/Release
                | ((static_cast<uint32_t>(InsData[4]) << 8) & 0x0000FF00) //Attack/Decay
                | ((static_cast<uint32_t>(InsData[0]) << 0) & 0x000000FF); //MultKEVA
            adl.carrier_E862 =
                ((static_cast<uint32_t>(InsData[9] & 0x07) << 24) & 0xFF000000) //WaveForm
                | ((static_cast<uint32_t>(InsData[7]) << 16) & 0x00FF0000) //Sustain/Release
                | ((static_cast<uint32_t>(InsData[5]) << 8) & 0x0000FF00) //Attack/Decay
                | ((static_cast<uint32_t>(InsData[1]) << 0) & 0x000000FF); //MultKEVA
            adl.modulator_40 = InsData[2];
            adl.carrier_40   = InsData[3];
            adl.feedconn     = InsData[10] & 0x0F;
            adl.finetune = 0;
            adlins.adl[0] = adl;
            adlins.adl[1] = adl;
            adlins.ms_sound_kon  = 1000;
            adlins.ms_sound_koff = 0;
            adlins.tone  = 0;
            adlins.flags = 0;
            adlins.voice2_fine_tune = 0.0;
        }

        m_synth.m_embeddedBank = OPL3::CustomBankTag; // Ignore AdlBank number, use dynamic banks instead
        //std::printf("CMF deltas %u ticks %u, basictempo = %u\n", deltas, ticks, basictempo);
        m_synth.m_rhythmMode = true;
        m_synth.m_musicMode = OPL3::MODE_CMF;
        m_synth.m_volumeScale = OPL3::VOLUME_NATIVE;
    }
    else if(format == MidiSequencer::Format_RSXX)
    {
        //opl.CartoonersVolumes = true;
        m_synth.m_musicMode     = OPL3::MODE_RSXX;
        m_synth.m_volumeScale   = OPL3::VOLUME_NATIVE;
    }
    else if(format == MidiSequencer::Format_IMF)
    {
        //std::fprintf(stderr, "Done reading IMF file\n");
        m_synth.m_numFourOps  = 0; //Don't use 4-operator channels for IMF playing!
        m_synth.m_musicMode = OPL3::MODE_IMF;
    }

    m_synth.reset(m_setup.emulator, m_setup.PCM_RATE, this); // Reset OPL3 chip
    //opl.Reset(); // ...twice (just in case someone misprogrammed OPL3 previously)
    m_chipChannels.clear();
    m_chipChannels.resize(m_synth.m_numChannels);

    return true;
}

bool MIDIplay::LoadMIDI(const std::string &filename)
{
    FileAndMemReader file;
    file.openFile(filename.c_str());
    if(!LoadMIDI_pre())
        return false;
    if(!m_sequencer.loadMIDI(file))
    {
        errorStringOut = m_sequencer.getErrorString();
        return false;
    }
    if(!LoadMIDI_post())
        return false;
    return true;
}

bool MIDIplay::LoadMIDI(const void *data, size_t size)
{
    FileAndMemReader file;
    file.openData(data, size);
    if(!LoadMIDI_pre())
        return false;
    if(!m_sequencer.loadMIDI(file))
    {
        errorStringOut = m_sequencer.getErrorString();
        return false;
    }
    if(!LoadMIDI_post())
        return false;
    return true;
}

#endif /* ADLMIDI_DISABLE_MIDI_SEQUENCER */
