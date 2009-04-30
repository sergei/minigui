TOP_DIR=../..

INCLUDES = -I../../include -I../include

noinst_LTLIBRARIES = libial.la

if NATIVE_IAL_ENGINE
SUBDIRS=native
endif

DIST_SUBDIR=native

EXTRA_DIST=spec_general_ts_drv.html

COMMON_SRCS = ial.c

DUMMY_SRCS = dummy.c dummy.h
AUTO_SRCS = auto.c auto.h
RANDOM_SRCS = random.c random.h
CUSTOM_SRCS = custom.c custom.h
QVFB_SRCS = qvfb.c qvfb.h
COMMINPUT_SRCS = comminput.c comminput.h
WVFB_SRCS = wvfbial.c wvfb.h winial.c winial.h
DFB_SRCS = dfb.c dfb.h

EP7211_SRCS = ep7211.c ep7211.h ep7211_internal.h
ADS_SRCS = ads.c ads.h ads_internal.h
IPAQ_SRCS = ipaq.c ipaq.h
L7200_SRCS = l7200.c l7200.h
ARM3000_SRCS = arm3000.c arm3000.h
EMBEST2410_SRCS = embest2410.c embest2410.h
DM270_SRCS = dm270.c dm270.h
EM8620_SRCS = em8620.c em8620.h
EM86_SRCS = em86.c em86.h libmc.h 
EM85_SRCS = em85.c em85.h
EVMV10_SRCS = evmv10.c evmv10.h
MPC823_SRCS = mpc823.c mpc823.h
PX255B_SRCS = px255b.c px255b.h
UCB1X00_SRCS = ucb1x00.c ucb1x00.h
VR4181_SRCS = vr4181.c vr4181.h
TPHELIO_SRCS = helio.c helio.h
TFSTB_SRCS = tf-stb.c tf-stb.h tf-code.h
T800_SRCS = t800.c t800.h
MC68X328_SRCS = mc68x328.c mc68x328.h
SMDK2410_SRCS = 2410.c 2410.h
DMGSTB_SRCS = dmg-stb.c dmg-stb.h
FIP_SRCS = fip.c fip.h
PALMII_SRCS = palm2.c palm2.h
HH2410R3_SRCS = hh2410r3.c hh2410r3.h
C33L05_SRCS = c33l05.c c33l05.h
HH2440_SRCS = hh2440.c hh2440.h
EMBEST44B0_SRCS = embest44b0.c embest44b0.h
SVPXX_SRCS = svpxx.h svpxx.c
ADS7846_SRCS = ads7846.h ads7846.c
HH5249KBDIR_SRCS = hh5249kbdir.c hh5249kbdir.h ps2-keycode.h ps2-conv.c
UTPMC_SRCS = utpmc.c utpmc.h
FFT7202_SRCS = fft7202.c fft7202.h
FXRM9200_SRCS = fxrm9200.c fxrm9200.h
SKYEYE_EP7312_SRCS = skyeye-ep7312.c skyeye-ep7312.h
FIGUER_SRCS = figueroa.c figueroa.h
HI3510_SRCS = hi3510.c hi3510.h

if NATIVE_IAL_ENGINE
libial_la_LIBADD = native/libnative.la
endif

if ABSSIG_IAL_ENGINE
ABSSIG_SRCS = abssig.c abssig.h abs_signal_driver.c abs_signal_driver.h
else
ABSSIG_SRCS = 
endif

libial_la_SOURCES = $(COMMON_SRCS) $(DUMMY_SRCS) $(AUTO_SRCS) $(RANDOM_SRCS) $(CUSTOM_SRCS) \
                    $(QVFB_SRCS) $(COMMINPUT_SRCS) $(WVFB_SRCS) $(DFB_SRCS) \
                    $(EM85_SRCS) $(EM86_SRCS) $(EM8620_SRCS) $(ARM3000_SRCS) \
                    $(EP7211_SRCS) $(ADS_SRCS) $(TPHELIO_SRCS) $(IPAQ_SRCS) \
                    $(L7200_SRCS)  $(VR4181_SRCS) $(TFSTB_SRCS)  \
                    $(T800_SRCS) $(MPC823_SRCS) $(PX255B_SRCS) \
                    $(MC68X328_SRCS) $(SMDK2410_SRCS) $(DMGSTB_SRCS) \
                    $(UCB1X00_SRCS) $(FIP_SRCS) \
                    $(PALMII_SRCS) $(HH2410R3_SRCS) $(C33L05_SRCS)  $(HH2440_SRCS) \
                    $(EMBEST44B0_SRCS) $(SVPXX_SRCS) \
                    $(EMBEST2410_SRCS) $(ADS7846_SRCS) $(HH5249KBDIR_SRCS) \
                    $(UTPMC_SRCS) $(FFT7202_SRCS) $(DM270_SRCS) \
                    $(EVMV10_SRCS) $(FXRM9200_SRCS) \
                    $(ABSSIG_SRCS) $(SKYEYE_EP7312_SRCS) $(FIGUER_SRCS) \
					$(HI3510_SRCS) 

