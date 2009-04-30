/*
** $Id: ial.c 8055 2007-11-07 08:26:03Z lbjiao $
** 
** The Input Abstract Layer of MiniGUI.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/06/13
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "minigui.h"
#include "misc.h"
#include "ial.h"

/* General IAL engiens ... */
#ifdef _DUMMY_IAL
    #include "dummy.h"
#endif
#ifdef _AUTO_IAL
    #include "auto.h"
#endif
#ifdef _RANDOM_IAL
    #include "random.h"
#endif
#ifdef _CUSTOM_IAL
    #include "custom.h"
#endif
#ifdef _COMM_IAL
    #include "comminput.h"
#endif
#ifdef _QVFB_IAL
    #include "qvfb.h"
#endif
#ifdef _WVFB_IAL
    #include "wvfb.h"
#endif
#ifdef _NATIVE_IAL_ENGINE
    #include "native/native.h"
#endif
#ifdef _DFB_IAL
    #include "dfb.h"
#endif
/* ... end of general IAL engiens */

/* Board-specific IAL engiens ... */
#ifdef _EP7211_IAL
    #include "ep7211.h"
#endif
#ifdef _ADS_IAL
    #include "ads.h"
#endif
#ifdef _IPAQ_IAL
    #include "ipaq.h"
#endif
#ifdef _FIGUEROA_IAL
    #include "figueroa.h"
#endif
#ifdef _VR4181_IAL
    #include "vr4181.h"
#endif
#ifdef _HELIO_IAL
    #include "helio.h"
#endif
#ifdef _TFSTB_IAL
    #include "tf-stb.h"
#endif
#ifdef _T800_IAL
    #include "t800.h"
#endif
#ifdef _HH5249KBDIR_IAL
    #include "hh5249kbdir.h"
#endif
#ifdef _MPC823_IAL
    #include "mpc823.h"
#endif
#ifdef _UCB1X00_IAL
    #include "ucb1x00.h"
#endif
#ifdef _PX255B_IAL
    #include "px255b.h"
#endif
#ifdef _MC68X328_IAL
    #include "mc68x328.h"
#endif
#ifdef _SMDK2410_IAL
    #include "2410.h"
#endif

#ifdef _EMBEST2410_IAL
    #include "embest2410.h"
#endif
#ifdef _EMBEST44B0_IAL
    #include "embest44b0.h"
#endif
#ifdef _DMGSTB_IAL
    #include "dmg-stb.h"
#endif
#ifdef _FIP_IAL
    #include "fip.h"
#endif
#ifdef _PALMII_IAL
    #include "palm2.h"
#endif
#ifdef _HH2410R3_IAL
    #include "hh2410r3.h"
#endif
#ifdef _C33L05_IAL
    #include "c33l05.h"
#endif
#ifdef _HH2440_IAL
    #include "hh2440.h"
#endif
#ifdef _SVPXX_IAL
    #include "svpxx.h"
#endif
#ifdef _ADS7846_IAL
    #include "ads7846.h"
#endif
#ifdef _EM8620_IAL
    #include "em8620.h"
#endif
#ifdef _EM86_IAL
    #include "em86.h"
#endif
#ifdef _EM85_IAL
    #include "em85.h"
#endif
#ifdef _L7200_IAL
    #include "l7200.h"
#endif
#ifdef _UTPMC_IAL
    #include "utpmc.h"
#endif
#ifdef _ARM3000_IAL
    #include "arm3000.h"
#endif
#ifdef _FFT7202_IAL
    #include "fft7202.h"
#endif
#ifdef _DM270_IAL
    #include "dm270.h"
#endif
#ifdef _EVMV10_IAL
    #include "evmv10.h"
#endif
#ifdef _FXRM9200_IAL
    #include "fxrm9200.h"
#endif
#ifdef _ABSSIG_IAL
    #include "abssig.h"
#endif
#ifdef _SKYEYE_EP7312_IAL
    #include "skyeye-ep7312.h"
#endif

#ifdef _HI3510_IAL
    #include "hi3510.h"
#endif
#ifdef _HI3560_IAL
    #include "hi3560.h"
#endif

/* ... end of board-specific IAL engiens */

#define LEN_ENGINE_NAME        16
#define LEN_MTYPE_NAME         16

static INPUT inputs [] = 
{
/* General IAL engines ... */
#ifdef _DUMMY_IAL
    {"dummy", InitDummyInput, TermDummyInput},
#endif
#ifdef _AUTO_IAL
    {"auto", InitAutoInput, TermAutoInput},
#endif
#ifdef _RANDOM_IAL
    {"random", InitRandomInput, TermRandomInput},
#endif
#ifdef _CUSTOM_IAL
    {"custom", InitCustomInput, TermCustomInput},
#endif
#ifdef _COMM_IAL
    {"comm", InitCOMMInput, TermCOMMInput},
#endif
#ifdef _QVFB_IAL
    {"qvfb", InitQVFBInput, TermQVFBInput},
#endif
#ifdef _WVFB_IAL
    {"wvfb", InitWVFBInput, TermWVFBInput}, 
#endif
#ifdef _NATIVE_IAL_ENGINE
    {"console", InitNativeInput, TermNativeInput},
#endif
#ifdef _DFB_IAL
    {"dfb", InitDFBInput, TermDFBInput},
#endif
/* ... end of general IAL engines */

/* Board-specific IAL engines... */
#ifdef _ADS_IAL
    {"ADS", InitADSInput, TermADSInput},
#endif
#ifdef _VR4181_IAL
    {"VR4181", InitVR4181Input, TermVR4181Input},
#endif
#ifdef _HELIO_IAL
    {"Helio", InitHelioInput, TermHelioInput},
#endif
#ifdef _EP7211_IAL
    {"EP7211", InitEP7211Input, TermEP7211Input},
#endif
#ifdef _TFSTB_IAL
    {"TF-STB", InitTFSTBInput, TermTFSTBInput},
#endif
#ifdef _HH5249KBDIR_IAL
    {"hh5249kbdir", InitHH5249KbdIrInput, TermHH5249KbdIrInput},
#endif
#ifdef _IPAQ_IAL
    {"ipaq", InitIPAQInput, TermIPAQInput},
#endif
#ifdef _T800_IAL
    {"T800", InitT800Input, TermT800Input},
#endif
#ifdef _MPC823_IAL
    {"MPC823", InitMPC823Input, TermMPC823Input},
#endif
#ifdef _UCB1X00_IAL
    {"UCB1X00", InitUCB1X00Input, TermUCB1X00Input},
#endif
#ifdef _PX255B_IAL
    {"PX255B", InitPX255BInput, TermPX255BInput},
#endif
#ifdef _MC68X328_IAL
    {"MC68X328", InitMC68X328Input, TermMC68X328Input},
#endif
#ifdef _SMDK2410_IAL
    {"SMDK2410", Init2410Input, Term2410Input},
#endif
#ifdef _DMGSTB_IAL
    {"dmg-stb", InitDMGSTBInput, TermDMGSTBInput},
#endif
#ifdef _FIP_IAL
    {"fip", InitFIPInput, TermFIPInput},
#endif
#ifdef _PALMII_IAL
    {"palm2", InitPALMIIInput, TermPALMIIInput},
#endif
#ifdef _HH2410R3_IAL
    {"hh2410r3", InitHH2410R3Input, TermHH2410R3Input},
#endif
#ifdef _C33L05_IAL
    {"c33l05", InitC33L05Input, TermC33L05Input},
#endif
#ifdef _HH2440_IAL
    {"hh2440", InitHH2440Input, TermHH2440Input},
#endif
#ifdef _EMBEST44B0_IAL
    {"embest44b0", InitEMBEST44b0Input, TermEMBEST44b0Input},
#endif
#ifdef _SVPXX_IAL
    {"svpxx", InitSvpxxInput, TermSvpxxInput},
#endif
#ifdef _ADS7846_IAL
    {"ads7846", InitAds7846Input, TermAds7846Input},
#endif
#ifdef _L7200_IAL
    {"l7200", InitL7200Input, TermL7200Input},
#endif
#ifdef _ARM3000_IAL
    {"arm3000", InitARM3000Input, TermARM3000Input},
#endif
#ifdef _EMBEST2410_IAL
    {"embest2410", InitEMBEST2410Input, TermEMBEST2410Input},
#endif
#ifdef _EM8620_IAL
    {"em8620", InitEm8620Input, TermEm8620Input},
#endif
#ifdef _EM86_IAL
    {"em86", InitEm86Input, TermEm86Input},
#endif
#ifdef _EM85_IAL
    {"em85", InitEm85Input, TermEm85Input},
#endif
#ifdef _FFT7202_IAL
    {"fft7202", InitFFTInput, TermFFTInput},
#endif
#ifdef _UTPMC_IAL
    {"utpmc", InitUTPMCInput, TermUTPMCInput},
#endif
#ifdef _DM270_IAL
    {"dm270", InitDM270Input, TermDM270Input},
#endif
#ifdef _EVMV10_IAL
    {"evmv10", InitXscaleEVMV10Input, TermXscaleEVMV10Input},
#endif
#ifdef _FXRM9200_IAL
    {"fxrm9200", InitRm9200Input, TermRm9200Input},
#endif
#ifdef _ABSSIG_IAL
    {"abssig", InitABSSIGInput, TermABSSIGInput},
#endif
#ifdef _SKYEYE_EP7312_IAL
    {"SkyEyeEP7312", InitSkyEyeEP7312Input, TermSkyEyeEP7312Input},
#endif
#ifdef _FIGUEROA_IAL
    {"figueroa", InitFiguerOAInput, TermFiguerOAInput},
#endif

#ifdef _HI3510_IAL
    {"hi3510", InitHI3510Input, TermHI3510Input},
#endif
#ifdef _HI3610_IAL
    {"hi3610", InitHI3610Input, TermHI3610Input},
#endif

/* ... end of board-specific IAL engines */
};

INPUT* __mg_cur_input;

#define NR_INPUTS  (sizeof (inputs) / sizeof (INPUT))

int InitIAL (void)
{
    int  i;
#ifndef __NOUNIX__
    char* env_value;
#endif
    char engine [LEN_ENGINE_NAME + 1];
    char mdev [MAX_PATH + 1];
    char mtype[LEN_MTYPE_NAME + 1];

    if (NR_INPUTS == 0)
        return ERR_NO_ENGINE;

#ifndef __NOUNIX__
    if ((env_value = getenv ("ial_engine"))) {
        strncpy (engine, env_value, LEN_ENGINE_NAME);
        engine [LEN_ENGINE_NAME] = '\0';
    }
    else
#endif
    if (GetMgEtcValue ("system", "ial_engine", engine, LEN_ENGINE_NAME) < 0)
        return ERR_CONFIG_FILE;

#ifndef __NOUNIX__
    if ((env_value = getenv ("mdev"))) {
        strncpy (mdev, env_value, MAX_PATH);
        mdev [MAX_PATH] = '\0';
    }
    else
#endif
    if (GetMgEtcValue ("system", "mdev", mdev, MAX_PATH) < 0)
        return ERR_CONFIG_FILE;

#ifndef __NOUNIX__
    if ((env_value = getenv ("mtype"))) {
        strncpy (mtype, env_value, LEN_MTYPE_NAME);
        mtype [LEN_MTYPE_NAME] = '\0';
    }
    else
#endif
    if (GetMgEtcValue ("system", "mtype", mtype, LEN_MTYPE_NAME) < 0)
        return ERR_CONFIG_FILE;

    for (i = 0; i < NR_INPUTS; i++) {
        if (strncmp (engine, inputs[i].id, LEN_ENGINE_NAME) == 0) {
            __mg_cur_input = inputs + i;
            break;
        }
    }
   
    if (__mg_cur_input == NULL) {
        fprintf (stderr, "IAL: Does not find the request engine: %s.\n", engine);
        if (NR_INPUTS) {
            __mg_cur_input = inputs;
            fprintf (stderr, "IAL: Use the first engine: %s\n", __mg_cur_input->id);
        }
        else
            return ERR_NO_MATCH;
    }

    strcpy (__mg_cur_input->mdev, mdev);

    if (!IAL_InitInput (__mg_cur_input, mdev, mtype)) {
        fprintf (stderr, "IAL: Init IAL engine failure.\n");
        return ERR_INPUT_ENGINE;
    }

#ifdef _DEBUG
    fprintf (stderr, "IAL: Use %s engine.\n", __mg_cur_input->id);
#endif

    return 0;
}

void TerminateIAL (void)
{
    IAL_TermInput ();
}

#ifdef _MISC_MOUSECALIBRATE

DO_MOUSE_CALIBRATE_PROC __mg_do_mouse_calibrate;
POINT __mg_mouse_org_pos;

void GUIAPI GetOriginalMousePosition (int* x, int* y)
{
    *x = __mg_mouse_org_pos.x;
    *y = __mg_mouse_org_pos.y;
}

#define NR_EQUATIONS    6
#define LSHIFT(x)       ((x)<<10)
#define RSHIFT(x)       ((x)>>12)

static int vars1 [NR_EQUATIONS], vars2 [NR_EQUATIONS], vars3 [NR_EQUATIONS], vars4 [NR_EQUATIONS];

static void do_mouse_calibrate (int* x, int* y)
{
    int x1, y1, x2, y2, x3, y3, x4, y4;

    x1 = vars1 [0] * (*x) + vars1 [1] * (*y) + vars1 [2];
    y1 = vars1 [3] * (*x) + vars1 [4] * (*y) + vars1 [5];

    x2 = vars2 [0] * (*x) + vars2 [1] * (*y) + vars2 [2];
    y2 = vars2 [3] * (*x) + vars2 [4] * (*y) + vars2 [5];

    x3 = vars3 [0] * (*x) + vars3 [1] * (*y) + vars3 [2];
    y3 = vars3 [3] * (*x) + vars3 [4] * (*y) + vars3 [5];

    x4 = vars4 [0] * (*x) + vars4 [1] * (*y) + vars4 [2];
    y4 = vars4 [3] * (*x) + vars4 [4] * (*y) + vars4 [5];

    *x = RSHIFT (x1 + x2 + x3 + x4);
    *y = RSHIFT (y1 + y2 + y3 + y4);
}

#if 0
static double coef_const [NR_EQUATIONS][NR_EQUATIONS + 1];

static void forwardElimination (void)
{
     int i, j, k;

     for (i = 0; i < NR_EQUATIONS; i++)
          for (j = i; j< NR_EQUATIONS; j++)
               for (k = NR_EQUATIONS; k > i; k--)
                    coef_const [j][k] = coef_const [j][k] - coef_const [i][k] * coef_const [j][i]/coef_const [i][i];

}

static void backwardSubstitute (double* x)
{
     int j,k;
     double t;

     for (j = NR_EQUATIONS - 1; j > 0; j--) {
          t = 0.0;

          for (k = j; k < NR_EQUATIONS; k++)
               t = t + coef_const [j][k] * x[k];

          x[j] = (coef_const [j][NR_EQUATIONS] - t) / coef_const [j][j];
     }
}

static BOOL doGaussianElimination (double* x, const POINT* src_pts, const POINT* dst_pts)
{
    int i, j;

    for (i = 0; i < NR_EQUATIONS; i++) {
        for (j = 0; j <= NR_EQUATIONS; j++)
            coef_const [i][j] = 0.0;
    }

    coef_const [0][0] = src_pts [0].x;
    coef_const [0][1] = src_pts [0].y;
    coef_const [0][2] = 1.0;
    coef_const [0][6] = dst_pts [0].x;

    coef_const [1][3] = src_pts [0].x;
    coef_const [1][4] = src_pts [0].y;
    coef_const [1][5] = 1.0;
    coef_const [1][6] = dst_pts [0].y;

    coef_const [2][0] = src_pts [1].x;
    coef_const [2][1] = src_pts [1].y;
    coef_const [2][2] = 1.0;
    coef_const [2][6] = dst_pts [1].x;

    coef_const [3][3] = src_pts [1].x;
    coef_const [3][4] = src_pts [1].y;
    coef_const [3][5] = 1.0;
    coef_const [3][6] = dst_pts [1].y;

    coef_const [4][0] = src_pts [2].x;
    coef_const [4][1] = src_pts [2].y;
    coef_const [4][2] = 1.0;
    coef_const [4][6] = dst_pts [2].x;

    coef_const [5][3] = src_pts [2].x;
    coef_const [5][4] = src_pts [2].y;
    coef_const [5][5] = 1.0;
    coef_const [5][6] = dst_pts [2].y;

    forwardElimination ();
    backwardSubstitute (x);

    return TRUE;
}

static BOOL doGaussianElimination_f (double* x, const POINT* src_pts, const POINT* dst_pts)
{
    double x12, x23, y12, y23, nx12, nx23, ny12, ny23;
    double numerator, denominator1, denominator2;

    x12 = (double)(src_pts [0].x - src_pts [1].x);
    x23 = (double)(src_pts [1].x - src_pts [2].x);
    y12 = (double)(src_pts [0].y - src_pts [1].y);
    y23 = (double)(src_pts [1].y - src_pts [2].y);

    nx12 = (double)(dst_pts [0].x - dst_pts [1].x);
    nx23 = (double)(dst_pts [1].x - dst_pts [2].x);
    ny12 = (double)(dst_pts [0].y - dst_pts [1].y);
    ny23 = (double)(dst_pts [1].y - dst_pts [2].y);

    denominator1 = x12*y23 - x23*y12;
    if (fabs (denominator1) < 0.0E-10)
        return FALSE;

    denominator2 = y12*x23 - y23*x12;
    if (fabs (denominator2) < 0.0E-10)
        return FALSE;

    numerator = nx12*y23 - nx23*y12;
    x [0] = numerator / denominator1;
    numerator = nx12*x23 - nx23*x12;
    x [1] = numerator / denominator2;
    x [2] = dst_pts [0].x - x [0] * src_pts [0].x - x [1] * src_pts [0].y;
    
    numerator = ny12*y23 - ny23*y12;
    x [3] = numerator / denominator1;
    numerator = ny12*x23 - ny23*x12;
    x [4] = numerator / denominator2;
    x [5] = dst_pts [0].y - x [3] * src_pts [0].x - x [4] * src_pts [0].y;

    return TRUE;
}

#else

static BOOL doGaussianElimination (int* x, const POINT* src_pts, const POINT* dst_pts)
{
    int x12, x23, y12, y23, nx12, nx23, ny12, ny23;
    int numerator, denominator1, denominator2;

    x12 = (src_pts [0].x - src_pts [1].x);
    x23 = (src_pts [1].x - src_pts [2].x);
    y12 = (src_pts [0].y - src_pts [1].y);
    y23 = (src_pts [1].y - src_pts [2].y);

    nx12 = (dst_pts [0].x - dst_pts [1].x);
    nx23 = (dst_pts [1].x - dst_pts [2].x);
    ny12 = (dst_pts [0].y - dst_pts [1].y);
    ny23 = (dst_pts [1].y - dst_pts [2].y);

    denominator1 = x12*y23 - x23*y12;
    if (denominator1 == 0)
        return FALSE;

    denominator2 = y12*x23 - y23*x12;
    if (denominator2 == 0)
        return FALSE;

    numerator = nx12*y23 - nx23*y12;
    x [0] = LSHIFT (numerator) / denominator1;
    numerator = nx12*x23 - nx23*x12;
    x [1] = LSHIFT (numerator) / denominator2;
    x [2] = LSHIFT (dst_pts [0].x) - x [0] * src_pts [0].x - x [1] * src_pts [0].y;
    
    numerator = ny12*y23 - ny23*y12;
    x [3] = LSHIFT (numerator) / denominator1;
    numerator = ny12*x23 - ny23*x12;
    x [4] = LSHIFT (numerator) / denominator2;
    x [5] = LSHIFT (dst_pts [0].y) - x [3] * src_pts [0].x - x [4] * src_pts [0].y;

    return TRUE;
}

#endif

BOOL GUIAPI SetMouseCalibrationParameters (const POINT* src_pts, const POINT* dst_pts)
{
    POINT my_src_pts [3];
    POINT my_dst_pts [3];

    my_src_pts [0] = src_pts [0];
    my_src_pts [1] = src_pts [1];
    my_src_pts [2] = src_pts [4];
    my_dst_pts [0] = dst_pts [0];
    my_dst_pts [1] = dst_pts [1];
    my_dst_pts [2] = dst_pts [4];
    if (!doGaussianElimination (vars1, my_src_pts, my_dst_pts))
        return FALSE;

    my_src_pts [0] = src_pts [1];
    my_src_pts [1] = src_pts [2];
    my_src_pts [2] = src_pts [4];
    my_dst_pts [0] = dst_pts [1];
    my_dst_pts [1] = dst_pts [2];
    my_dst_pts [2] = dst_pts [4];
    if (!doGaussianElimination (vars2, my_src_pts, my_dst_pts))
        return FALSE;

    my_src_pts [0] = src_pts [2];
    my_src_pts [1] = src_pts [3];
    my_src_pts [2] = src_pts [4];
    my_dst_pts [0] = dst_pts [2];
    my_dst_pts [1] = dst_pts [3];
    my_dst_pts [2] = dst_pts [4];
    if (!doGaussianElimination (vars3, my_src_pts, my_dst_pts))
        return FALSE;

    my_src_pts [0] = src_pts [0];
    my_src_pts [1] = src_pts [3];
    my_src_pts [2] = src_pts [4];
    my_dst_pts [0] = dst_pts [0];
    my_dst_pts [1] = dst_pts [3];
    my_dst_pts [2] = dst_pts [4];
    if (!doGaussianElimination (vars4, my_src_pts, my_dst_pts))
        return FALSE;

    __mg_do_mouse_calibrate = do_mouse_calibrate;
    return TRUE;
}

#endif /* _MISC_MOUSECALIBRATE */

