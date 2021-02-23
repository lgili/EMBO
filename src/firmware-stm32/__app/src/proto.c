/*
 * CTU/EMBO - EMBedded Oscilloscope <github.com/parezj/EMBO>
 * Author: Jakub Parez <parez.jakub@gmail.com>
 */


#include "cfg.h"
#include "proto.h"

#include "app_data.h"
#include "main.h"
#include "utility.h"

#include "FreeRTOS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/************************* [IEEE 488] *************************/

scpi_result_t EM_Reset(scpi_t * context)
{
    daq_enable(&daq, EM_FALSE);
    daq_settings_init(&daq);
    daq_mode_set(&daq, VM);
    daq_enable(&daq, EM_TRUE);
    cntr_enable(&cntr, EM_FALSE);
    pwm_disable(&pwm);
#ifdef EM_DAC
    sgen_disable(&sgen);
#endif

#ifdef EM_DEBUG
    pwm_set(&pwm, 1000, 25, 25, 50, EM_TRUE, EM_TRUE); // TODO to fn
#ifdef EM_DAC
    sgen_enable(&sgen, SINE, 100, 1000, EM_DAC_BUFF_LEN);
#endif
#endif

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

/************************* [System Actions] *************************/

scpi_result_t EM_SYS_Mode(scpi_t * context)
{
    const char* p1;
    size_t p1l;

    if (!SCPI_ParamCharacters(context, &p1, &p1l, TRUE))
        return SCPI_RES_ERR;

    if (strcmp(p1, "SCOPE\r\n") == 0)
        daq_mode_set(&daq, SCOPE);
    else if (strcmp(p1, "VM\r\n") == 0)
        daq_mode_set(&daq, VM);
    else if (strcmp(p1, "LA\r\n") == 0)
        daq_mode_set(&daq, LA);
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

scpi_result_t EM_SYS_ModeQ(scpi_t * context)
{
    if (daq.mode == SCOPE)
        SCPI_ResultText(context, "SCOPE");
    else if (daq.mode == VM)
        SCPI_ResultText(context, "VM");
    else
        SCPI_ResultText(context, "LA");

    return SCPI_RES_OK;
}

scpi_result_t EM_SYS_LimitsQ(scpi_t * context)
{
    char buff[80];
    char dual[2] = {'\0'};
    char inter[2] = {'\0'};
    uint8_t dac = 0;
    uint8_t bit8 = 0;
    uint8_t adcs = 0;
#ifdef EM_DAC
    dac = 1;
#endif
#ifdef EM_ADC_BIT8
    bit8 = 1;
#endif

#if defined(EM_ADC_MODE_ADC1)
    adcs = 1;
#elif defined(EM_ADC_MODE_ADC12)
    adcs = 2;
#elif defined(EM_ADC_MODE_ADC1234)
    adcs = 4;
#endif

#if defined(EM_ADC_DUALMODE)
    dual[0] = 'D';
#endif
#if defined(EM_ADC_INTERLEAVED)
    inter[0] = 'I';
#endif

    //char smplt12_s[15];
    //char smplt8_s[15];
    //sprint_fast(smplt12_s, "%s", EM_ADC_1CH_SMPL_TM(EM_ADC_SMPLT_MAX_N, EM_ADC_TCONV12), 8);
    //sprint_fast(smplt8_s, "%s", EM_ADC_1CH_SMPL_TM(EM_ADC_SMPLT_MAX_N, EM_ADC_TCONV8), 8);

    int pwm_max_f = EM_TIM_PWM1_FREQ / 2;

    int len = sprintf(buff, "%d,%d,%d,%d,%d,%d%s%s,%d,%d", EM_DAQ_MAX_B12_FS, EM_DAQ_MAX_B8_FS, EM_DAQ_MAX_MEM,
                      EM_LA_MAX_FS, pwm_max_f, adcs, dual, inter, bit8, dac);

    SCPI_ResultCharacters(context, buff, len);
    return SCPI_RES_OK;
}

/************************* [VM Actions] *************************/

scpi_result_t EM_VM_ReadQ(scpi_t * context)
{
    if (daq.mode == VM)
    {
        daq_enable(&daq, EM_FALSE);

        uint32_t p1 = 0;
        if (context != NULL)
            SCPI_ParamUInt32(context, &p1, FALSE);
        else
            p1 = 1;

        float vref_raw = 0;
        float ch1_raw = 0;
        float ch2_raw = 0;
        float ch3_raw = 0;
        float ch4_raw = 0;

        int avg_num = 1;
        if (p1 > 0)
            avg_num = (int)p1;

        if (avg_num > daq.set.mem)
        {
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        }

#if defined(EM_ADC_MODE_ADC1)
        int last1 = EM_DMA_LAST_IDX(daq.buff1.len, EM_DMA_CH_ADC1, EM_DMA_ADC1);

        get_avg_from_circ(last1, 5, avg_num, daq.buff1.len, daq.buff1.data, daq.set.bits, &vref_raw, &ch1_raw, &ch2_raw, &ch3_raw, &ch4_raw);

#elif defined(EM_ADC_MODE_ADC12)
        int last1 = EM_DMA_LAST_IDX(daq.buff1.len, EM_DMA_CH_ADC1, EM_DMA_ADC1);
        int last2 = EM_DMA_LAST_IDX(daq.buff2.len, EM_DMA_CH_ADC2, EM_DMA_ADC2);

        get_avg_from_circ(last1, 3, avg_num, daq.buff1.len, daq.buff1.data, daq.set.bits, &vref_raw, &ch1_raw, &ch2_raw, NULL, NULL);
        get_avg_from_circ(last2, 2, avg_num, daq.buff2.len, daq.buff2.data, daq.set.bits, &ch3_raw, &ch4_raw, NULL, NULL, NULL);

#elif defined(EM_ADC_MODE_ADC1234)
        int last1 = EM_DMA_LAST_IDX(daq.buff1.len, EM_DMA_CH_ADC1, EM_DMA_ADC1);
        int last2 = EM_DMA_LAST_IDX(daq.buff2.len, EM_DMA_CH_ADC2, EM_DMA_ADC2);
        int last3 = EM_DMA_LAST_IDX(daq.buff3.len, EM_DMA_CH_ADC3, EM_DMA_ADC3);
        int last4 = EM_DMA_LAST_IDX(daq.buff4.len, EM_DMA_CH_ADC4, EM_DMA_ADC4);

        get_avg_from_circ(last1, 2, avg_num, daq.buff1.len, daq.buff1.data, daq.set.bits, &vref_raw, &ch1_raw, NULL, NULL, NULL);
        get_avg_from_circ(last2, 1, avg_num, daq.buff2.len, daq.buff2.data, daq.set.bits, &ch2_raw, NULL, NULL, NULL, NULL);
        get_avg_from_circ(last3, 1, avg_num, daq.buff3.len, daq.buff3.data, daq.set.bits, &ch3_raw, NULL, NULL, NULL, NULL);
        get_avg_from_circ(last4, 1, avg_num, daq.buff4.len, daq.buff4.data, daq.set.bits, &ch4_raw, NULL, NULL, NULL, NULL);
#endif

        char vcc_s[10];
        char ch1_s[10];
        char ch2_s[10];
        char ch3_s[10];
        char ch4_s[10];

#ifdef VREFINT_CAL_ADDR
        float vcc = 3.3 * EM_ADC_VREF_CAL / vref_raw;
#else
        float vcc = daq.adc_max_val * EM_ADC_VREF_CAL / vref_raw / 1000;
#endif
        float ch1 = vcc * ch1_raw / daq.adc_max_val;
        float ch2 = vcc * ch2_raw / daq.adc_max_val;
        float ch3 = vcc * ch3_raw / daq.adc_max_val;
        float ch4 = vcc * ch4_raw / daq.adc_max_val;

        daq.vref = vref_raw;
        daq.vcc_mv = vcc * 1000;

        if (context == NULL)
        {
            if (daq.vcc_mv > 0)
                return SCPI_RES_OK;
            else
                return SCPI_RES_ERR;
        }

        sprint_fast(vcc_s, "%s", vcc, 4);
        sprint_fast(ch1_s, "%s", ch1, 4);
        sprint_fast(ch2_s, "%s", ch2, 4);
        sprint_fast(ch3_s, "%s", ch3, 4);
        sprint_fast(ch4_s, "%s", ch4, 4);

        char buff[100];
        int len = sprintf(buff, "%s,%s,%s,%s,%s", ch1_s, ch2_s, ch3_s, ch4_s, vcc_s);

        daq_enable(&daq, EM_TRUE);

        SCPI_ResultCharacters(context, buff, len);
        return SCPI_RES_OK;
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

/************************* [SCOPE Actions] *************************/

scpi_result_t EM_SCOPE_ReadQ(scpi_t * context)
{
    if (daq.mode == SCOPE)
    {
        if (daq.trig.ready == 0)
        {
            SCPI_ResultText(context, EM_RESP_NRDY);
            return SCPI_RES_OK;
        }

        if (daq.trig.set.mode == DISABLED)
            daq_enable(&daq, EM_FALSE);

#ifdef VREFINT_CAL_ADDR
        float cal = EM_ADC_VREF_CAL / daq.adc_max_val * 3300;
#else
        float cal = EM_ADC_VREF_CAL;
#endif

        if (daq.set.bits == B8) // compressing
            cal /= 10.0;
        else
            cal *= 10.0;

#if defined(EM_ADC_MODE_ADC1)
        int added = 0;
        int idx = 0;
        int ch_it = 1; // 2 /w Vcc

        int buff1_mem = daq.buff1.len - daq.buff1.reserve;

        if (daq.trig.set.mode == DISABLED)
            daq.trig.pos_frst = EM_DMA_LAST_IDX(daq.buff1.len, daq.trig.dma_ch_trig, daq.trig.dma_trig);

        if (daq.set.ch1_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);
        if (daq.set.ch2_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);
        if (daq.set.ch3_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);
        if (daq.set.ch4_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);

#elif defined(EM_ADC_MODE_ADC12)

        int added = 0;
        int idx = 0;
        int ch_it = 1; // 2 /w Vcc

        int buff1_mem = daq.buff1.len - daq.buff1.reserve;
        int buff2_mem = daq.buff2.len - daq.buff2.reserve;

        if (daq.trig.set.mode == DISABLED)
            daq.trig.pos_frst = EM_DMA_LAST_IDX((daq.set.ch1_en || daq.set.ch2_en) ? daq.buff1.len : daq.buff2.len,
                daq.trig.dma_ch_trig, daq.trig.dma_trig);

        if (daq.set.ch1_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);
        if (daq.set.ch2_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, ch_it++, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);

        ch_it = 1;
        if (daq.set.ch3_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff2_mem, daq.buff2.len, ch_it++, daq.buff2.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff2.data, daq.buff_out.data, &idx);
        if (daq.set.ch4_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff2_mem, daq.buff2.len, ch_it++, daq.buff2.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff2.data, daq.buff_out.data, &idx);

#elif defined(EM_ADC_MODE_ADC1234)

        int buff1_mem = daq.buff1.len - daq.buff1.reserve;
        int buff2_mem = daq.buff2.len - daq.buff2.reserve;
        int buff3_mem = daq.buff3.len - daq.buff3.reserve;
        int buff4_mem = daq.buff4.len - daq.buff4.reserve;

        int added = 0;
        int idx = 0;

        if (daq.trig.set.mode == DISABLED)
        {
            uint16_t buff_ln = daq.buff1.len;
            if (daq.set.ch2_en) buff_ln = daq.buff2.len;
            else if (daq.set.ch3_en) buff_ln = daq.buff3.len;
            else if (daq.set.ch4_en) buff_ln = daq.buff4.len;

            daq.trig.pos_frst = EM_DMA_LAST_IDX(buff_ln, daq.trig.dma_ch_trig, daq.trig.dma_trig);
        }

        if (daq.set.ch1_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff1_mem, daq.buff1.len, 1, daq.buff1.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff1.data, daq.buff_out.data, &idx);
        if (daq.set.ch2_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff2_mem, daq.buff2.len, 1, daq.buff2.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff2.data, daq.buff_out.data, &idx);
        if (daq.set.ch3_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff3_mem, daq.buff3.len, 1, daq.buff3.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff3.data, daq.buff_out.data, &idx);
        if (daq.set.ch4_en)
            added += get_1ch_from_circ(daq.trig.pos_frst, buff4_mem, daq.buff4.len, 1, daq.buff4.chans,
                                       daq.set.bits, daq.vref, cal, daq.buff4.data, daq.buff_out.data, &idx);
#endif

        daq.trig.pretrig_cntr = 0;
        daq.trig.ready = 0;
        daq.trig.ready_last = 0;

        if (daq.trig.set.mode != SINGLE)
        {
            daq_enable(&daq, EM_TRUE);
        }

        //ASSERT(added == daq.buff_out.len);
        if (added != daq.buff_out.len)
        {
            SCPI_ResultText(context, "ERROR: invalid buffer size"); // TODO DEBUG
            return SCPI_RES_OK;
        }

        SCPI_ResultArbitraryBlock(context, daq.buff_out.data, daq.buff_out.len);
        return SCPI_RES_OK;
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_SCOPE_Set(scpi_t * context)
{
    if (daq.mode == SCOPE)
    {
        uint32_t p1, p2, p3, p5, p6, p9;
        const char* p4;
        const char* p7;
        const char* p8;
        size_t p4l, p7l, p8l;

        if (!SCPI_ParamUInt32(context, &p1, TRUE) ||
            !SCPI_ParamUInt32(context, &p2, TRUE) ||
            !SCPI_ParamUInt32(context, &p3, TRUE) ||
            !SCPI_ParamCharacters(context, &p4, &p4l, TRUE) ||
            !SCPI_ParamUInt32(context, &p5, TRUE) ||
            !SCPI_ParamUInt32(context, &p6, TRUE) ||
            !SCPI_ParamCharacters(context, &p7, &p7l, TRUE) ||
            !SCPI_ParamCharacters(context, &p8, &p8l, TRUE) ||
            !SCPI_ParamUInt32(context, &p9, TRUE))
        {
            return SCPI_RES_ERR;
        }

        if (p4l != 4 || p7l != 1 || p8l != 1 ||
            (p4[0] != 'T' && p4[0] != 'F') || (p4[1] != 'T' && p4[1] != 'F') ||
            (p4[2] != 'T' && p4[2] != 'F') || (p4[3] != 'T' && p4[3] != 'F') ||
            (p7[0] != 'R' && p7[0] != 'F') || (p8[0] != 'A' && p8[0] != 'N' && p8[0] != 'S' && p8[0] != 'D'))
        {
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        }

        daq_settings_save(&daq.set, &daq.trig.set, &daq.save_s, &daq.trig.save_s);
        daq_enable(&daq, EM_FALSE);
        daq_reset(&daq);
        daq.dis_hold = 1;

        daq_mem_set(&daq, 3); // safety guard
        int ret2 = daq_bit_set(&daq, (int)p1);
        int ret4 = daq_ch_set(&daq, p4[0] == 'T' ? 1 : 0, p4[1] == 'T' ? 1 : 0,
                                    p4[2] == 'T' ? 1 : 0, p4[3] == 'T' ? 1 : 0, (int)p3);
        int ret3 = daq_fs_set(&daq, (int)p3);
        int ret1 = daq_mem_set(&daq, (int)p2);
        int ret5 = daq_trig_set(&daq, p5, p6, (p7[0] == 'R' ? RISING : FALLING),
            (p8[0] == 'A' ? AUTO : (p8[0] == 'N' ? NORMAL : (p8[0] == 'S' ? SINGLE : DISABLED))), (int)p9);

        if (ret1 + ret2 + ret3 + ret4 + ret5 == 0)
        {
            daq.dis_hold = 0;
            daq_enable(&daq, EM_TRUE);

            SCPI_ResultText(context, SCPI_OK);
            return SCPI_RES_OK;
        }
        else
        {
            daq.mode = VM;
            daq_mode_set(&daq, SCOPE);  // reload saved settings
            daq.dis_hold = 0;
            daq_enable(&daq, EM_TRUE);

            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        }
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_SCOPE_SetQ(scpi_t * context)
{
    if (daq.mode == SCOPE)
    {
        char buff[80];
        char freq_s[30];
        char chans_en[5];
        char edge_s[2];
        char mode_s[2];

        sprint_fast(freq_s, "%s", daq.set.fs, 3);
        chans_en[0] = daq.set.ch1_en ? 'T' : 'F';
        chans_en[1] = daq.set.ch2_en ? 'T' : 'F';
        chans_en[2] = daq.set.ch3_en ? 'T' : 'F';
        chans_en[3] = daq.set.ch4_en ? 'T' : 'F';
        edge_s[0] = daq.trig.set.edge == RISING ? 'R' : 'F';
        mode_s[0] = daq.trig.set.mode == AUTO ? 'A' : (daq.trig.set.mode == NORMAL ? 'N' :
            (daq.trig.set.mode == SINGLE ? 'S' : 'D'));
        chans_en[4] = '\0';
        edge_s[1] = '\0';
        mode_s[1] = '\0';

        char maxZ_s[15];
        float max_Z = EM_ADC_MAXZ(daq.smpl_time, daq.set.bits == B12 ? EM_LN2POW14 : EM_LN2POW10);
        sprint_fast(maxZ_s, "%skOhm", max_Z, 1);

        int len = sprintf(buff, "\"%d,%d,%s,%s,%d,%d,%s,%s,%d,%s\"", daq.set.bits, daq.set.mem, freq_s, chans_en,
                          daq.trig.set.ch, daq.trig.set.val_percent, edge_s, mode_s, daq.trig.set.pretrigger, maxZ_s);

        SCPI_ResultCharacters(context, buff, len);
        return SCPI_RES_OK;
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_SCOPE_ForceTrig(scpi_t * context)
{
    if (daq.mode != SCOPE)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }

    if (daq.trig.ready || daq.trig.set.mode == DISABLED || daq.trig.set.mode == AUTO)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_FUNCTION_NOT_AVAILABLE);
        return SCPI_RES_ERR;
    }

    daq.trig.dma_pos_catched = EM_DMA_LAST_IDX(daq.trig.buff_trig->len, daq.trig.dma_ch_trig, daq.trig.dma_trig);
    daq_trig_trigger_scope(&daq);

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

scpi_result_t EM_SCOPE_Average(scpi_t * context)
{
    // TODO
    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

/************************* [LA Actions] *************************/

scpi_result_t EM_LA_ReadQ(scpi_t * context)
{
    if (daq.mode == LA)
    {
        if (daq.trig.ready == 0)
        {
            SCPI_ResultText(context, EM_RESP_NRDY);
            return SCPI_RES_OK;
        }

        if (daq.trig.set.mode == DISABLED)
        {
            daq_enable(&daq, EM_FALSE);
            daq.trig.pos_frst = EM_DMA_LAST_IDX(daq.buff1.len, EM_DMA_CH_LA, EM_DMA_LA);
        }

        for (int k = 0, i = daq.trig.pos_frst; k < daq.buff1.len; k++, i++) // TODO compress 4 + 4
        {
            if (i >= daq.buff1.len)
                i = 0;

            uint8_t val = (uint8_t)(((uint8_t*)daq.buff1.data)[i]);
            ((uint8_t*)daq.buff_out.data)[k] = (((val & (1 << EM_GPIO_LA_CH1_NUM)) ? 1 : 0) << 1) |
                                               (((val & (1 << EM_GPIO_LA_CH2_NUM)) ? 1 : 0) << 2) |
                                               (((val & (1 << EM_GPIO_LA_CH3_NUM)) ? 1 : 0) << 3) |
                                               (((val & (1 << EM_GPIO_LA_CH4_NUM)) ? 1 : 0) << 4);
        }

        daq.trig.pretrig_cntr = 0;
        daq.trig.ready = 0;
        daq.trig.ready_last = 0;

        if (daq.trig.set.mode != SINGLE)
        {
            daq_enable(&daq, EM_TRUE);
        }

        SCPI_ResultArbitraryBlock(context, daq.buff_out.data, daq.buff_out.len);
        return SCPI_RES_OK;
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_LA_Set(scpi_t * context)
{
    if (daq.mode == LA)
    {
        uint32_t p2, p3, p5, p9;
        const char* p7;
        const char* p8;
        size_t p7l, p8l;

        if (!SCPI_ParamUInt32(context, &p2, TRUE) ||
            !SCPI_ParamUInt32(context, &p3, TRUE) ||
            !SCPI_ParamUInt32(context, &p5, TRUE) ||
            !SCPI_ParamCharacters(context, &p7, &p7l, TRUE) ||
            !SCPI_ParamCharacters(context, &p8,&p8l, TRUE) ||
            !SCPI_ParamUInt32(context, &p9, TRUE))
        {
            return SCPI_RES_ERR;
        }

        if (p7l != 1 || p8l != 1 ||
            (p7[0] != 'R' && p7[0] != 'F') ||
            (p8[0] != 'A' && p8[0] != 'N' && p8[0] != 'S' && p8[0] != 'D'))
        {
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        }

        daq_settings_save(&daq.set, &daq.trig.set, &daq.save_l, &daq.trig.save_l);
        daq_enable(&daq, EM_FALSE);
        daq_reset(&daq);
        daq.dis_hold = 1;

        daq_mem_set(&daq, 3); // safety guard
        int ret2 = daq_bit_set(&daq, B1);
        int ret4 = daq_ch_set(&daq, 1, 1, 1, 1, (int)p3);
        int ret3 = daq_fs_set(&daq, (int)p3);
        int ret1 = daq_mem_set(&daq, (int)p2);
        int ret5 = daq_trig_set(&daq, p5, 0, (p7[0] == 'R' ? RISING : FALLING),
            (p8[0] == 'A' ? AUTO : (p8[0] == 'N' ? NORMAL : (p8[0] == 'S' ? SINGLE : DISABLED))), (int)p9);

        if (ret1 + ret2 + ret3 + ret4 + ret5 == 0)
        {
            daq.dis_hold = 0;
            daq_enable(&daq, EM_TRUE);

            SCPI_ResultText(context, SCPI_OK);
            return SCPI_RES_OK;
        }
        else
        {
            daq.mode = VM;
            daq_mode_set(&daq, LA); // reload saved settings
            daq.dis_hold = 0;
            daq_enable(&daq, EM_TRUE);

            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
            return SCPI_RES_ERR;
        }
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_LA_SetQ(scpi_t * context)
{
    if (daq.mode == LA)
    {
        char buff[80];
        char freq_s[30];
        char edge_s[2];
        char mode_s[2];

        sprint_fast(freq_s, "%s", daq.set.fs, 3);
        edge_s[0] = daq.trig.set.edge == RISING ? 'R' : 'F';
        mode_s[0] = daq.trig.set.mode == AUTO ? 'A' : (daq.trig.set.mode == NORMAL ? 'N' :
            (daq.trig.set.mode == SINGLE ? 'S' : 'D'));
        edge_s[1] = '\0';
        mode_s[1] = '\0';

        int len = sprintf(buff, "\"%d,%s,%d,%s,%s,%d\"", daq.set.mem, freq_s,
                          daq.trig.set.ch, edge_s, mode_s, daq.trig.set.pretrigger);

        SCPI_ResultCharacters(context, buff, len);
        return SCPI_RES_OK;
    }
    else
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }
}

scpi_result_t EM_LA_ForceTrig(scpi_t * context)
{
    if (daq.mode != LA)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_MODE);
        return SCPI_RES_ERR;
    }

    if (daq.trig.ready || daq.trig.set.mode == DISABLED || daq.trig.set.mode == AUTO)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_FUNCTION_NOT_AVAILABLE);
        return SCPI_RES_ERR;
    }


    daq.trig.dma_pos_catched = EM_DMA_LAST_IDX(daq.trig.buff_trig->len, EM_DMA_CH_LA, EM_DMA_LA);
    daq_trig_trigger_la(&daq);

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

/************************* [CNTR Actions] *************************/

scpi_result_t EM_CNTR_Enable(scpi_t * context)
{
    uint32_t p1;

    if (!SCPI_ParamUInt32(context, &p1, TRUE))
    {
        return SCPI_RES_ERR;
    }

    if (p1 < 0 || p1 > 1)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    cntr_enable(&cntr, p1);

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}

scpi_result_t EM_CNTR_ReadQ(scpi_t * context)
{
    if (!cntr.enabled)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_CNTR_NOT_ENABLED);
        return SCPI_RES_ERR;
    }

    float f = cntr.freq;

    if (f > -1)
    {
        char f_s[20];
        char T_s[20];

        float T = 1.0 / f;

        if (f < 1000)
            sprint_fast(f_s, "%s Hz", f, 3);
        else if (f < 1000000)
            sprint_fast(f_s, "%s kHz", f / 1000.0, 3);
        else //if (f < 1000000000)
            sprint_fast(f_s, "%s MHz", f / 1000000.0, 3);

        if (T >= 1)
            sprint_fast(T_s, "%s s", T, 3);
        else if (T >= 0.001)
            sprint_fast(T_s, "%s ms", T * 1000.0, 3);
        else //if (T >= 0.000001)
            sprint_fast(T_s, "%s us", T * 1000000.0, 3);

        char buff[100];
        int len = sprintf(buff, "%s,%s", f_s, T_s);

        SCPI_ResultCharacters(context, buff, len);
        return SCPI_RES_OK;
    }
    else // if (f == -1)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_TIME_OUT);
        return SCPI_RES_ERR;
    }
}

/************************* [SGEN Actions] *************************/
scpi_result_t EM_SGEN_Set(scpi_t * context)
{
#ifdef EM_DAC
    double p1;
    if (!SCPI_ParamDouble(context, &p1, TRUE))
    {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    //sgen_enable(&sgen, p1 == 1); // TODO

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
#else
    SCPI_ErrorPush(context, SCPI_ERROR_DAC_NA);
    return SCPI_RES_ERR;
#endif
}

/************************* [PWM Actions] *************************/

scpi_result_t EM_PWM_SetQ(scpi_t * context)
{
    char buff[60];
    char buff_freq[10];
    char buff_duty1[10];
    char buff_duty2[10];

    float freq = pwm.ch1.freq;
    float duty1 = pwm.ch1.duty;
    float duty2 = pwm.ch2.duty;
    int offset2 = pwm.ch2.offset;
    uint8_t en1 = pwm.ch1.enabled;
    uint8_t en2 = pwm.ch2.enabled;

    sprint_fast(buff_freq, "%s", freq, 4);
    sprint_fast(buff_duty1, "%s", duty1, 4);
    sprint_fast(buff_duty2, "%s", duty2, 4);
    int len = sprintf(buff, "%s,%s,%s,%d,%d,%d", buff_freq, buff_duty1, buff_duty2, offset2, en1, en2);

    SCPI_ResultCharacters(context, buff, len);

    return SCPI_RES_OK;
}

scpi_result_t EM_PWM_Set(scpi_t * context)
{
    uint32_t param1, param2, param3, param4, param5, param6;

    if (!SCPI_ParamUInt32(context, &param1, TRUE) ||
        !SCPI_ParamUInt32(context, &param2, TRUE) ||
        !SCPI_ParamUInt32(context, &param3, TRUE) ||
        !SCPI_ParamUInt32(context, &param4, TRUE) ||
        !SCPI_ParamUInt32(context, &param5, TRUE) ||
        !SCPI_ParamUInt32(context, &param6, TRUE))
    {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    int ret = pwm_set(&pwm, param1, param2, param3, param4, param5, param6);

    if (ret != 0)
    {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        return SCPI_RES_ERR;
    }

    SCPI_ResultText(context, SCPI_OK);
    return SCPI_RES_OK;
}