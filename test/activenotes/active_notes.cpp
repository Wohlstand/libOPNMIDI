#include <catch.hpp>
#include <string>
#include <vector>

#include "opnmidi_midiplay.hpp"
#include "opnmidi_opn2.hpp"
#include "opnmidi_private.hpp"

TEST_CASE( "MIDI Channel manipulating", "[OPNMIDIplay::MIDIchannel]" )
{
    OPNMIDIplay::MIDIchannel midi_ch;
    //OPNMIDIplay::OpnChannel  opn_ch; //Must be tested separately, but togerther with MIDI Channel as there are in symbiosis

    SECTION("Turning notes On!")
    {
        for(uint8_t noteq = 0; noteq < 128; noteq++)
        {
            uint8_t note = rand() % 128;
            {//Call note off if note is already in work
                OPNMIDIplay::MIDIchannel::notes_iterator
                i = midi_ch.find_activenote(note);
                if(!i.is_end())
                    midi_ch.activenotes.erase(i);
            }

            const OpnInstMeta &ains = Synth::m_emptyInstrument;

            OPNMIDIplay::MIDIchannel::NoteInfo::Phys voices[OPNMIDIplay::MIDIchannel::NoteInfo::MaxNumPhysChans] = {
                {0, ains.op[0], /*false*/},
                {0, ains.op[1], /*pseudo_4op*/},
            };

            OPNMIDIplay::MIDIchannel::notes_iterator ir;
            ir = midi_ch.ensure_find_or_create_activenote(note);
            OPNMIDIplay::MIDIchannel::NoteInfo &ni = ir->value;
            ni.vol     = rand() % 127;
            ni.noteTone    = rand() % 127;
            ni.midiins = rand() % 127;
            ni.currentTone = (double)(rand() % 127);
            ni.glideRate = (double)(rand() % 127);
            ni.ains = &ains;
            ni.chip_channels_count = 0;

            for(unsigned ccount = 0; ccount < OPNMIDIplay::MIDIchannel::NoteInfo::MaxNumPhysChans; ++ccount)
            {
                int32_t c = rand() % 2;
                if(c < 0)
                    continue;
                uint16_t chipChan = static_cast<uint16_t>(rand() % 256);
                OPNMIDIplay::MIDIchannel::NoteInfo::Phys * p = ni.phys_ensure_find_or_create(chipChan);
                REQUIRE( p != nullptr );
                p->assign(voices[ccount]);
            }
        }
    }

    SECTION("Turning another random notes Off!")
    {
        for(uint8_t noteq = 0; noteq < 128; noteq++)
        {
            uint8_t note = rand() % 128;
            OPNMIDIplay::MIDIchannel::notes_iterator
            i = midi_ch.find_activenote(note);
            if(!i.is_end())
                midi_ch.activenotes.erase(i);
        }
    }

    SECTION("Iterating notes are left!")
    {
        for(OPNMIDIplay::MIDIchannel::notes_iterator i = midi_ch.activenotes.begin(); !i.is_end();)
        {
            OPNMIDIplay::MIDIchannel::notes_iterator j(i++);
            REQUIRE( !j.is_end() );
        }
    }
}

