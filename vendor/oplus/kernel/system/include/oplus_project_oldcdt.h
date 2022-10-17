/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */
#ifndef _OPPO_PROJECT_OLDCDT_H_
#define _OPPO_PROJECT_OLDCDT_H_

enum{
    HW_VERSION__UNKNOWN,
    HW_VERSION__10,		//EVB
    HW_VERSION__11,		//T0
    HW_VERSION__12,		//EVT-1
    HW_VERSION__13,		//EVT-2
    HW_VERSION__14,		//DVT-1
    HW_VERSION__15,		//DVT-2
    HW_VERSION__16,		//PVT
    HW_VERSION__17,		//MP
    HW_VERSION_MAX,
};

enum{
    RF_VERSION__UNKNOWN,
    RF_VERSION__11,
    RF_VERSION__12,
    RF_VERSION__13,
    RF_VERSION__21,
    RF_VERSION__22,
    RF_VERSION__23,
    RF_VERSION__31,
    RF_VERSION__32,
    RF_VERSION__33,
};

#define GET_PCB_VERSION_OLDCDT() (get_PCB_Version_oldcdt())
#define GET_PCB_VERSION_STRING_OLDCDT() (get_PCB_Version_String_oldcdt())

#define GET_MODEM_VERSION_OLDCDT() (get_Modem_Version_oldcdt())
#define GET_OPERATOR_VERSION_OLDCDT() (get_Operator_Version_oldcdt())

enum OPPO_PROJECT_OLDCDT {
    OPPO_UNKOWN = 0,
	OPPO_17331 = 17331,
	OPPO_17197 = 17197,
	OPPO_17061 = 17061,
	OPPO_17175 = 17175,
    OPPO_17311 = 17311,
    OPPO_17321 = 17321,
    OPPO_17323 = 17323,
    OPPO_17325 = 17325,
    OPPO_17031 = 17031,
    OPPO_17032 = 17032,
    OPPO_17051 = 17051,
    OPPO_17052 = 17052,
    OPPO_17053 = 17053,
    OPPO_17055 = 17055,
    OPPO_17071 = 17071,
    OPPO_17073 = 17073,//17073 custom
    OPPO_17371 = 17371,
    OPPO_17373 = 17373,
    OPPO_17101 = 17101,
    OPPO_17102 = 17102,
    OPPO_17103 = 17103,
    OPPO_17105 = 17105,
    OPPO_17111 = 17111,
    OPPO_17113 = 17113,
    OPPO_17181 = 17181,
    OPPO_17182 = 17182,
    OPPO_17307 = 17307,
    OPPO_17309 = 17309,
    OPPO_17310 = 17310,
    OPPO_17381 = 17381,
    OPPO_18311 = 18311,
    OPPO_18011 = 18011,
    OPPO_18151 = 18151,
    OPPO_18531 = 18531,
    OPPO_18073 = 18073,
    OPPO_18593 = 18593,
    OPPO_19011 = 19011,
    OPPO_19301 = 19301,
    OPPO_19350 = 19350,
    OPPO_19151 = 19151,
    OPPO_18161 = 18161,
    OPPO_18561 = 18561,
    OPPO_18611 = 18611,
    OPPO_19091 = 19091,
    OPPO_19391 = 19391,
    OPPO_19531 = 19531,
    OPPO_19550 = 19550,
    OPPO_19551 = 19551,
    OPPO_19553 = 19553,
    OPPO_19556 = 19556,
    OPPO_19597 = 19597,
    OPPO_19354 = 19354,
    OPPO_19357 = 19357,
    OPPO_19358 = 19358,
    OPPO_19359 = 19359,
    OPPO_19536 = 19536,
    OPPO_19537 = 19537,
    OPPO_19538 = 19538,
    OPPO_19539 = 19539,
    OPPO_19661 = 19661,
    OPPO_19741 = 19741,
    OPPO_19747 = 19747,
    OPPO_20682 = 20682,
    OPPO_206AC = 0x206AC,
    OPPO_206A1 = 0x206A1,
    OPPO_20730 = 20730,
    OPPO_20731 = 20731,
    OPPO_20732 = 20732,
    OPPO_20733 = 20733,
    OPPO_20701 = 20701,
};

enum OPPO_OPERATOR {
    OPERATOR_UNKOWN                   	= 0,
    OPERATOR_17131_CHINA_OPEN_MARKET    = 1,
    OPERATOR_17198_MOBILE             	= 2,
    OPERATOR_CHINA_UNICOM             	= 3,
    OPERATOR_CHINA_TELECOM            	= 4,
    OPERATOR_17335_17197_ALL_BAND		= 5,
    OPERATOR_17332_ALL_BAND_HIGH_CONFIG = 6,
    OPERATOR_FOREIGN_OPEN_MARKET     	= 7,
    OPERATOR_17331_ASIA             	= 8,
    OPERATOR_17199_TAIWAN         		= 9,
    OPERATOR_FOREIGN_INDIA            	= 10,
    OPERATOR_FOREIGN_MALAYSIA         	= 11,
    OPERATOR_FOREIGN_EU               	= 12,
    OPERATOR_17331_EVB		      	  	= 13,
    OPERATOR_17337_ASIA				  	= 14,
    OPERATOR_17338_INDIA			  	= 15,
    OPERATOR_17340_ALL_BAND			  	= 16,
    OPERATOR_17339_ALL_BAND_HIGH_CONFIG = 17,
    OPERATOR_17061_CHINA				= 18,
    OPERATOR_17061_MOBILE				= 19,
    OPERATOR_17061_ASIA					= 20,
    OPERATOR_17062_ASIA_SIMPLE			= 21,
    OPERATOR_17063_TAIAO				= 22,
    OPERATOR_17175_ALL_BAND				= 23,
    OPERATOR_17176_MOBILE				= 24,
    OPERATOR_17179_FOREIGN				= 25,
    OPERATOR_17180_FOREIGN				= 26,
    OPERATOR_17341_INDIA				= 27,
    OPERATOR_17171_ASIA_SIMPLE			= 28,
    OPERATOR_17172_ASIA					= 29,
    OPERATOR_17173_ALL_BAND				= 30,
    OPERATOR_17065_INDIA				= 31,
    OPERATOR_17067_CHINA				= 32,
    OPERATOR_17066_INDIA_HIGH_CONFIG	= 33,
    OPERATOR_FOREIGN_TAIWAN_17111       = 34,
    OPERATOR_ALL_TELECOM_CARRIER_17111  = 35,
    OPERATOR_ALL_MOBILE_CARRIER_17113   = 36,
    OPERATOR_18311_ASIA					= 37,
    OPERATOR_18312_INDIA				= 38,
    OPERATOR_18313_ASIA_ALL_BAND		= 39,
    OPERATOR_18011_CHINA_ALL_BAND		= 40,
    OPERATOR_18012_CMCC					= 41,
    OPERATOR_18317_VIETNAM				= 42,
    OPERATOR_18312_INDIA_EVB			= 43,
    OPERATOR_18012_CMCC_EVB				= 44,
    OPERATOR_18318_VIETNAM				= 45,
    OPERATOR_18328_ASIA_SIMPLE_NORMALCHG = 46,
    OPERATOR_18311_ASIA_2ND_RESOURCE	= 47,
    OPERATOR_18012_CMCC_2ND_RESOURCE	= 48,
    OPERATOR_18013_CHINA_ALL_BAND_FINGER = 49,
    OPERATOR_18015_CMCC_FINGER			= 50,
    /*for realme, start from 56*/
    OPERATOR_18531_ASIA_SIMPLE			= 56,
    OPERATOR_18531_ASIA					= 57,
    OPERATOR_18531_All_BAND				= 58,
    OPERATOR_18151_All_BAND				= 59,  // 18151
    OPERATOR_18151_MOBILE				= 60,  //18153
    OPERATOR_18151_CARRIER				= 61,  //18551
    OPERATOR_18161_ALL_BAND				= 62,
    OPERATOR_18161_ASIA					= 63,
    OPERATOR_18161_ASIA_SIMPLE			= 64,
    OPERATOR_18161_MOBILE				= 65,
    OPERATOR_18161_ALL_MODES			= 66,
    /*for 18522*/
    OPERATOR_18521_ASIA_SIMPLE			= 67,
    OPERATOR_18521_ASIA					= 68,
    OPERATOR_18521_All_BAND				= 69,
    /* for 18073&18593 */
    OPERATOR_18073_MOBILE				= 70,
    OPERATOR_18073_All_BAND				= 71,
    OPERATOR_18593_CARRIER				= 72,
    /* for 18525*/
    OPERATOR_18525_ASIA					= 73,
    /* for 18161 vietnam*/
    OPERATOR_18161_ASIA_64G				= 74,  // 18566
    OPERATOR_18161_ASIA_128G			= 75,  // 18567
    OPERATOR_18161_ASIA_6_128G			= 76,  // 18568
    OPERATOR_18161_ASIA_SIMPLE_INDIA	= 77,
    OPERATOR_19301_CARRIER				= 80,
    OPERATOR_19305_CARRIER				= 81,
    OPERATOR_19011_All_BAND				= 82,
    OPERATOR_19011_MOBILE				= 83,
    /* for Hodor*/
    OPERATOR_19091_ASIA					= 84,
    OPERATOR_19391_ASIA					= 85,
    /* for OceanLite*/
    OPERATOR_19531_ASIA_SIMPLE			= 86,
    OPERATOR_19531_ASIA					= 87,
    OPERATOR_19531_All_BAND				= 88,
	/*for rm nemo */
    OPERATOR_19661_ASIA_SIMPLE           = 101,
    OPERATOR_19661_RUSSIA             	 = 102,
    OPERATOR_19661_All_NET             	 = 103,
    OPERATOR_19661_All_BAND              = 104,
    OPERATOR_19661_All_WORLD		     = 105,
    OPERATOR_19661_VIETNAM_8X128G        = 106,
	/*for rm sarter */
    OPERATOR_19661_ASIA_SIMPLE_SARTER     =111,
    OPERATOR_19661_All_BAND_SARTER        = 112,
    OPERATOR_19661_VIETNAM_8X128G_SARTER  = 113,
	OPERATOR_19661_All_BAND_NFC_SARTER    = 114,
	/* for rm sala */
    OPERATOR_20682_ASIA_SIMPLE            = 140,
    OPERATOR_20682_All_BAND               = 141,
    OPERATOR_20682_All_BAND_VIETNAM       = 142,
	/* for rm sala-A */
    OPERATOR_20682_SALA_A_ASIA_SIMPLE         = 143,
    OPERATOR_20682_SALA_A_All_BAND            = 144,
    OPERATOR_20682_SALA_A_All_BAND_VIETNAM    = 145,
    OPERATOR_20682_SALA_All_BAND_NFC          = 146,
    OPERATOR_20682_SALA_A_INTERNATIONAL       = 147,
    /* for rm Pascal_A */
    OPERATOR_206AC_PASCAL_A_ASIA_SIMPLE         = 156,
    OPERATOR_206AC_PASCAL_A_All_BAND            = 157,
    /* for rm monet */
    OPERATOR_19741_All_BAND_Z             	  = 101,
    OPERATOR_19741_All_BAND                   = 111,
    OPERATOR_19741_ASIA_SIMPLE             	  = 112,
    OPERATOR_19741_RUSSIA             	      = 113,
    OPERATOR_19741_ASIA_SIMPLE_INDIA          = 114,
    OPERATOR_19741_VIETNAM_3X32G		      = 115,
    OPERATOR_19741_VIETNAM_3X64G              = 116,
    /* for rm monetX */
    OPERATOR_19747_ASIA_SIMPLE                = 121,
    OPERATOR_19747_RUSSIA             	      = 122,
    OPERATOR_19747_All_BAND             	  = 123,
    OPERATOR_19747_VIETNAM_3X64G              = 124,
    OPERATOR_19747_VIETNAM_4X128G		      = 125,
    OPERATOR_19747_TELECOM                    = 126,
    /* for rm pascal-E */
    OPERATOR_206A1_PASCAL_E_ALL_GLOBAL        =132,
    OPERATOR_206A1_PASCAL_E_ALL_VIETNAM_VNR   =135,
    OPERATOR_206A1_PASCAL_E_ASIA_SIMPLE       =136,
    OPERATOR_206A1_PASCAL_E_INTERNATIONAL     =137,
    OPERATOR_206A1_PASCAL_E_ALL_BAND          =138,
    OPERATOR_206A1_PASCAL_E_ALL_VIETNAM_3X64G =139,
    OPERATOR_206A1_PASCAL_E_ALL_BAND_VIETNAM  =140,
    /* for rm pascal */
    OPERATOR_206A1_PASCAL_ASIA_SIMPLE         =141,
    OPERATOR_206A1_PASCAL_ALL_BAND_NFC        =142,
    OPERATOR_206A1_PASCAL_ALL_BAND            =143,
    OPERATOR_206A1_PASCAL_ALL_VIETNAM_3X32G   =144,
    OPERATOR_206A1_PASCAL_ALL_VIETNAM_4X64G   =145,
    OPERATOR_206A1_PASCAL_ALL_VIETNAM_4X128G  =146,
   /* for rm RIO-D*/
   OPERATOR_20701_RIO_D_ASIA_SIMPLE           = 177,
   OPERATOR_20701_RIO_D_All_WORLD             = 178,
   OPERATOR_20701_RIO_D_All_BAND              = 179,
   OPERATOR_20701_RIO_D_THAILAND              = 180,
   OPERATOR_20701_RIO_D_All_BAND_VIETNAM      = 181,
   OPERATOR_20701_RIO_D_THAILAND_NFC          = 182,
   OPERATOR_20701_RIO_D_All_BAND_VDF_NFC      = 183,
   OPERATOR_20701_RIO_D_All_BAND_EU           = 184,
};

enum{
    SMALLBOARD_VERSION__0,
    SMALLBOARD_VERSION__1,
    SMALLBOARD_VERSION__2,
    SMALLBOARD_VERSION__3,
    SMALLBOARD_VERSION__4,
    SMALLBOARD_VERSION__5,
    SMALLBOARD_VERSION__6,
    SMALLBOARD_VERSION__UNKNOWN = 100,
};

typedef struct
{
    unsigned int    nProject;
    unsigned int    nModem;
    unsigned int    nOperator;
    unsigned int    nPCBVersion;
    unsigned int    nENGVersion;
    unsigned int    isConfidential;
} ProjectInfoCDTType_oldcdt;

unsigned int get_project_oldcdt(void);
unsigned int is_project_oldcdt(int project );
unsigned int get_PCB_Version_oldcdt(void);
unsigned int get_Modem_Version_oldcdt(void);
unsigned int get_Operator_Version_oldcdt(void);
unsigned int get_eng_version_oldcdt(void);
int is_confidential_oldcdt(void);
bool oppo_daily_build_oldcdt(void);
int oppo_project_init_oldcdt(void);

#endif
