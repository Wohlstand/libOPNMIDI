#include <wopn_file.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct FMPARAMETER_OP
{
    int
    AR, /*Attack*/          /* && p.op1.AR >= 0 && p.op1.AR <= 31 */
    DR, /*Decay 1*/         /* && p.op1.DR >= 0 && p.op1.DR <= 31 */
    SR, /*Decay 2*/         /* && p.op1.SR >= 0 && p.op1.SR <= 31 */
    RR, /*Release*/         /* && p.op1.RR >= 0 && p.op1.RR <= 15 */
    SL, /*Sustain*/         /* && p.op1.SL >= 0 && p.op1.SL <= 15 */
    TL, /*Total level*/     /* && p.op1.TL >= 0 && p.op1.TL <= 127 */
    KS, /*Key/Rate scale*/  /* && p.op1.KS >= 0 && p.op1.KS <= 3*/
    ML, /*Multiply*/        /* && p.op1.ML >= 0 && p.op1.ML <= 15 */
    DT, /*Detune*/          /* && p.op1.DT >= 0 && p.op1.DT <= 7 */
    AMS;/*AM sensitivity*/  /* && p.op1.AMS >= 0 && p.op1.AMS <= 3 */
};

/*
p.ALG = 4, p.FB = 3, p.LFO = 0,
1.AR = 26, 1.DR = 10, 1.SR = 1, 1.RR = 0, 1.SL = 0, 1.TL = 2, 1.KS = 0, 1.ML = 1, 1.DT = 3, 1.AMS = 0,
2.AR = 26, 2.DR = 10, 2.SR = 2, 2.RR = 7, 2.SL = 2, 2.TL = 0, 2.KS = 0, 2.ML = 2, 2.DT = 3, 2.AMS = 0,
3.AR = 26, 3.DR = 10, 3.SR = 2, 3.RR = 0, 3.SL = 0, 3.TL = 4, 3.KS = 0, 3.ML = 1, 3.DT = 7, 3.AMS = 0,
4.AR = 18, 4.DR = 6,  4.SR = 1, 4.RR = 6, 4.SL = 4, 4.TL = 2, 4.KS = 1, 4.ML = 1, 4.DT = 7, 4.AMS = 0;
*/

// FM sound source parameters.
struct FMPARAMETER
{
    int
    ALG, /*    p.ALG >= 0 && p.ALG <= 7 */
    FB,  /* && p.FB >= 0 && p.FB <= 7 */
    LFO; /* && p.LFO >= 0 && p.LFO <= 7 */
    struct FMPARAMETER_OP op1, op2, op3, op4;
    int key, panpot, assign;
};

static WOPNFile *g_file = NULL;

void note_factory_set_program_common(WOPNInstrument *ins, struct FMPARAMETER *p, int isdrum)
{
    struct FMPARAMETER_OP *in_ops[4] = {&p->op1, &p->op3, &p->op2, &p->op4};
    int i = 0;
    int tl_mods[4] = {0, 0, 0, 0};
    int tl_mod_coefficient = 24;

    ins->delay_on_ms = 2000;
    ins->delay_off_ms = 100;

    if(isdrum != 1)
    {
        if(isdrum == 2)
            tl_mod_coefficient = 14;
        if(isdrum == 3)
            tl_mod_coefficient = 15;
        switch(p->ALG)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            tl_mods[0] = tl_mod_coefficient; tl_mods[2] = tl_mod_coefficient; tl_mods[1] = tl_mod_coefficient; tl_mods[3] = 0; break;
        case 4:
            tl_mods[0] = tl_mod_coefficient; tl_mods[2] = 0;  tl_mods[1] = tl_mod_coefficient; tl_mods[3] = 0; break;
        case 5:
        case 6:
            tl_mods[0] = tl_mod_coefficient; tl_mods[2] = 0;  tl_mods[1] = 0; tl_mods[3] = 0; break;
        case 7:
            tl_mods[0] = 0; tl_mods[2] = 0;  tl_mods[1] = 0; tl_mods[3] = 0; break;
        }
    }

    /*
        uint8_t out = 0;
        out |= 0x38 & (uint8_t(feedback) << 3);
        out |= 0x07 & (uint8_t(algorithm));
    */
    ins->fbalg = 0;
    ins->fbalg |= 0x38 & ((uint8_t)(p->FB) << 3);
    ins->fbalg |= 0x07 & ((uint8_t)(p->ALG));

    /*
        uint8_t out = 0;
        out |= 0x30 & (uint8_t(am) << 4);
        out |= 0x07 & (uint8_t(fm));
    */
    ins->lfosens = 0;
    ins->lfosens |= 0x30 & ((uint8_t)(p->LFO == 0 ? in_ops[0]->AMS : p->LFO) << 4);



    for(i = 0; i < 4; i++)
    {
        /*
            uint8_t out = 0;
            out |= 0x70 & (uint8_t(OP[OpID].detune) << 4);
            out |= 0x0F & (uint8_t(OP[OpID].fmult));
        */
        ins->operators[i].dtfm_30 = 0;
        ins->operators[i].dtfm_30 |= 0x70 & ((uint8_t)(in_ops[i]->DT) << 4);
        ins->operators[i].dtfm_30 |= 0x0F & ((uint8_t)(in_ops[i]->ML));

        /*
            OP[OpID].level = reg_level & 0x7F;
        */
        ins->operators[i].level_40 = 0;
        ins->operators[i].level_40 |= (in_ops[i]->TL != 127 ? tl_mods[i] + in_ops[i]->TL : in_ops[i]->TL) & 0x7F;

        /*
            uint8_t out = 0;
            out |= 0xC0 & (uint8_t(OP[OpID].ratescale) << 6);
            out |= 0x1F & (uint8_t(OP[OpID].attack));
        */
        ins->operators[i].rsatk_50 = 0;
        ins->operators[i].rsatk_50 |= 0xC0 & ((uint8_t)(in_ops[i]->KS) << 6);
        ins->operators[i].rsatk_50 |= 0x1F & ((uint8_t)(in_ops[i]->AR));

        /*
            uint8_t out = 0;
            out |= 0x80 & (uint8_t(OP[OpID].am_enable) << 7);
            out |= 0x1F & (uint8_t(OP[OpID].decay1));
        */
        ins->operators[i].amdecay1_60 = 0;
        ins->operators[i].amdecay1_60 |= 0x80 & ((uint8_t)(in_ops[i]->AMS > 0 ? 1 : 0) << 7);
        ins->operators[i].amdecay1_60 |= 0x1F & ((uint8_t)(in_ops[i]->DR));

        /*
            uint8_t out = 0;
            out |= 0x1F & (uint8_t(OP[OpID].decay2));
        */
        ins->operators[i].decay2_70 = 0;
        ins->operators[i].decay2_70 |= 0x1F & (in_ops[i]->SR);

        /*
            uint8_t out = 0;
            out |= 0xF0 & (uint8_t(OP[OpID].sustain) << 4);
            out |= 0x0F & (uint8_t(OP[OpID].release));
        */
        ins->operators[i].susrel_80 = 0;
        ins->operators[i].susrel_80 |= 0xF0 & ((uint8_t)(in_ops[i]->SL) << 4);
        ins->operators[i].susrel_80 |= 0x0F & ((uint8_t)(in_ops[i]->RR) << 0);

        ins->operators[i].ssgeg_90 = 0;
        ins->operators[i].ssgeg_90 |= 0;
    }
}

void note_factory_set_program(int number, struct FMPARAMETER *p)
{
    WOPNInstrument *ins = &g_file->banks_melodic[0].ins[number];
    note_factory_set_program_common(ins, p, 0);
    if(number == 32) /* Fix the lower octave of Acoustic Bass */
        ins->note_offset = +12;
}

int avoidTlMod(int number)
{
    switch(number)
    {
        case 35:
        case 36: /* Kick */

        case 41: case 43: case 45: case 47: case 48: case 50: /* toms */
            return 0;

        case 51: /* Ride cymbal */
            return 3;

        case 42: /* Closed hi-hat */
        case 44:
        case 46: /* open hi-hat */
            return 2;

        default:
            return 1;
    }
}

void note_factory_set_drum_program(int number, struct FMPARAMETER *p)
{
    WOPNInstrument *ins = &g_file->banks_percussive[0].ins[number];
    if(number < 0)
        return;
    note_factory_set_program_common(ins, p, avoidTlMod(number));
    ins->percussion_key_number = p->key;
}

int main()
{
    struct FMPARAMETER p;
    size_t outSize = 0; FILE *out = NULL; uint8_t *out_block = NULL;
    g_file = WOPN_Init(1, 1);

    g_file->lfo_freq = 2;
    g_file->chip_type = WOPN_Chip_OPNA;

    #include "midiprogram.h"

    outSize = WOPN_CalculateBankFileSize(g_file, g_file->version);
    out_block = (uint8_t*)malloc(outSize);
    WOPN_SaveBankToMem(g_file, out_block, outSize, g_file->version, 0);
    out = fopen("fmmidi.wopn", "wb");
    fwrite(out_block, 1, outSize, out);
    fclose(out);
    WOPN_Free(g_file);

    return 0;
}

