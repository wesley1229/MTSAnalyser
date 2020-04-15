/********************************************
*      MPEG-2 Transport Stream PSI analyzer *
*      Wesley Chen 2013-2020          *
*      Software Verion 1.1.0           *
********************************************/
#ifndef __PSIPARSER__
#define __PSIPARSER__

/*Debug definition*/
#define DUMPPAT

/*Type definition*/
#define uchar unsigned char
#define uint unsigned int
#define ushort unsigned short
typedef enum {false, true} bool; 

/*TS Packet header info*/
#define SYNCBYTE 0x47
#define TSPKLEN 188 
#define NORMAL_SECTION_LENGTH 1024

/*TS header Mask*/
#define TRANSPORT_BIT_MASK 0x1
#define PAYLOAD_BIT_MASK 0x1
#define PRIORITY_BIT_MASK 0x1
#define PID_MASK 0x1fff
#define SCRAMBLE_BIT_MASK 0x3
#define ADAPTATION_FIELD_MASK 0x3
#define CONTINUITY_CONNTER_MASK 0xf

/*PAT MASK*/
#define SECTION_LEN_MASK 0xfff
#define VERSION_MASK 0x1f
#define CURRENT_NEXT_MASK 0x1
#define PMT_PID_MASK 0x1fff

/*PMT MASK*/
#define PCR_PID_MASK 0x1fff
#define PROGRAM_INFO_MASK 0xfff
#define ES_PID_MASK 0x1fff
#define ES_LEN_MASK 0xfff

/*NIT MASK*/
#define PROTOCOL_MASK 0x1f
#define SUBTYPE_MASK 0xf
typedef enum {CDS = 1, MMS} NIT_TYPE; 
/*CDS MASK*/
#define SPACEUNIT_MASK 0x1
#define FREQUENCYSPACE_MASK 0x3fff
#define FIRST_CARRIER_FREQUENCY_MASK 0x7fff

/*MMS MASK*/
#define INNER_CODE_MASK 0xf
#define MODULATION_MASK 0x1f
#define SYMBOLRATE_MASK 0xfffffff

/*DCM MASK*/
#define FIRST_VCH_MASK 0xfff
#define DCM_LENGTH_MASK 0x7f

/*VCM MASK*/
#define DESCRIPTORS_INCLUDED_MASK 0x1
#define VIRTUAL_CHANNEL_MASK 0xfff
#define PATH_SELECT_MASK 0x1
#define TRANSPORT_TYPE_MASK 0x1
#define CHANNEL_TYPE_MASK 0xf
#define VIDEO_STANDARD_MASK 0xf

/*EAS MASK*/
#define SEQUENCE_NUMBER_MASK 0x1f
#define ALERT_PRIORITY_MASK 0xf
#define DETAIL_CHANNEL_MASK 0x3ff
#define COUNTY_CODE_MASK 0x3ff
#define DESCRIPTOR_COUNT_MASK 0x3ff
/*********************************
*           MEPG-2 standard structure          *
*********************************/

/*
* TS header structure
*/
typedef struct TSPK
{
      uchar error_bit;
      uchar playload_start_bit;
      uchar priority_bit;
      uint PID;
	uchar scramble_control;
	uchar adaption_field_control;
	uchar continuity_counter;
}  TSPK_HEADER;

/*
* PAT structure
*/
typedef struct TSPK_pat
{
      uchar table_id;
      ushort section_lenth;
      ushort stream_id;
      uchar version_number;
	uchar next_indicator;
	uchar section_number;
	uchar last_section_number;
	ushort program_number[20];
	ushort PMT_PID[20];
	uint CRC;
	uchar PMT_Num;
}  TSPK_PAT;

/*
* ES structure
*/
typedef struct TSPK_es
{
      uchar stream_type;
	ushort ES_PID;
	ushort ES_INFO_LEN; //outer descriptor
}TSPK_ES;

/*
* PMT structure
*/
typedef struct TSPK_pmt
{
      ushort PMT_PID;
      uchar table_id;
      ushort section_lenth;
      ushort program_number;
      uchar version_number;
	uchar next_indicator;
	uchar section_number;
	uchar last_section_number; 
	ushort PCR_PID;
	ushort programInfo_len;  //inner descriptor
      TSPK_ES es_data[256];
	uint CRC;
	uchar ES_Num;
	struct TSPK_pmt *next;
}  TSPK_PMT;

/*
* CAT structure
*/
typedef struct TSPK_cat
{
      uchar table_id;
      ushort section_lenth;
      uchar version_number;
	uchar next_indicator;
	uchar section_number;
	uchar last_section_number;
	ushort outer_descriptor_num; //outer descriptor
	uint CRC;
}  TSPK_CAT;

/*
* TSDT structure
*/
typedef struct TSPK_tsdt
{
      uchar table_id;
      ushort section_lenth;
      uchar version_number;
	uchar next_indicator;
	uchar section_number;
	uchar last_section_number;
	ushort outer_descriptor_num; //outer descriptor
	uint CRC;
}  TSPK_TSDT;

/*********************************
*           SCTE 65 element structure          *
*********************************/

/*
* SCTE65 - CDS data structure
*/
typedef struct SCTE65_cds
{
      uchar number_of_carriers;
	uchar spacing_unit;
	ushort frequency_spacing;
	uchar frequency_unit;
	ushort first_carrier_frquency;
}SCTE65_CDS;

/*
* SCTE65 - MMS data structure
*/
typedef struct SCTE65_mms
{
      uchar transmission_system;
	uchar inner_coding_mode;
	uchar split_bitstream_mode;
	uchar modulation_format;
	uint symbol_rate;
}SCTE65_MMS;

/*
* SCTE65 - SNS data structure
*/
typedef struct SCTE65_sns
{
	uchar application_type;
	ushort application_source_id;
	uchar name_length;
	uchar source_name[256];
	ushort SNS_descriptor_count;
	
}SCTE65_SNS;

/*
* SCTE65 - DCM Data structure
*/
typedef struct DCM_data
{
	uchar range_defined;
	uchar channels_count;
}DCM_DATA;

/*
* SCTE65 - VCM Virtual Channel structure
*/
typedef struct VCM_virtualch
{
	ushort virtual_channel_number;
	uchar application_virtual_channel;
	uchar path_select;
	uchar transport_type;
	uchar channel_type;
	ushort application_source_id;
	uchar CDS_reference;
	ushort program_number;
	uchar MMS_reference;
	bool scrambled;
	uchar video_standard;
	uchar descriptors_count; //inner descriptor
} VCM_VirtualCH;

/*
* SCTE65 - ICM Data structure
*/
typedef struct ICM_data
{
	ushort source_id;
	ushort virtual_channel_number;
}ICM_DATA;

/*
* SCTE18 - Location code structure
*/
typedef struct location_code
{
	uchar state_code;
	uchar county_subdivision;
	ushort county_code;
}LOCATION_CODE;

/*
* SCTE18 - exception data structure
*/
typedef struct exception
{
	bool in_band_reference;
	ushort exception_major_channel_number;
	ushort exception_minor_channel_number;
	ushort exception_OOB_source_ID;
}EXCEPTION;

/*********************************
*            SCTE 65 main structure             *
*********************************/
/*
* SCTE65 - NIT-CDS structure
*/
typedef struct SCTE_nit_cds
{
      ushort NIT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
      uchar first_index;
	uchar number_of_records;
	uchar transmission_medium;
	uchar table_subtype;
	SCTE65_CDS cds_data[256];
	uchar CDS_Num;
	uchar descriptor_count; //inner descriptor
	ushort outer_descriptor_num; //outer descriptor
	uint CRC;
}  SCTE_NIT_CDS;

/*
* SCTE65 - NIT-MMS structure
*/
typedef struct SCTE_nit_mms
{
      ushort NIT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
      uchar first_index;
	uchar number_of_records;
	uchar transmission_medium;
	uchar table_subtype;
	SCTE65_MMS mms_data[256];
	uchar MMS_Num;
	uchar descriptor_count;  //inner descriptor
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_NIT_MMS;

/*
* SCTE65 - NTT-SNS structure
*/
typedef struct SCTE_ntt_sns
{
      ushort NTT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
      uchar iso639_language_code[4];
	uchar transmission_medium;
	uchar table_subtype;
	uchar number_of_sns;
	SCTE65_SNS sns_data[256];
	uchar SNS_Num;
	uchar descriptor_count;  //inner descriptor
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_NTT_SNS;

/*
* SCTE65 - SVCT-DCM structure
*/
typedef struct SCTE_svct_dcm
{
      ushort SVCT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
	uchar transmission_medium;
	uchar table_subtype;
	ushort VCT_ID;
	ushort first_virtual_channel;
	uchar DCM_data_length;
	DCM_DATA dcm_data[127];
	uchar DCM_Num;
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_SVCT_DCM;

/*
* SCTE65 - SVCT-VCM structure
*/
typedef struct SCTE_svct_vcm
{
      ushort SVCT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
	uchar transmission_medium;
	uchar table_subtype;
	ushort VCT_ID;
	uchar descriptors_included;
	uchar splice;
	uint activation_time;
	uchar number_of_VC_records;
	VCM_VirtualCH Virtual_CH[256];
	uchar VCM_Num;
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_SVCT_VCM;

/*
* SCTE65 - SVCT-ICM structure
*/
typedef struct SCTE_svct_icm
{
      ushort SVCT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
	uchar transmission_medium;
	uchar table_subtype;
	ushort VCT_ID;
	ushort first_map_index;
	uchar record_count;
	ICM_DATA icm_data[128];
	uchar ICM_Num;
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_SVCT_ICM;

/*
* SCTE65 - NTT-STT structure
*/
typedef struct SCTE_stt
{
      ushort STT_PID;
      uchar table_id;
      ushort section_lenth;
      uchar protocol_version;
      uint system_time;
	uchar GPS_UTC_offset;
	ushort outer_descriptor_num;  //outer descriptor
	uint CRC;
}  SCTE_STT;

/*
* SCTE18 - EAS structure
*/
typedef struct SCTE_eas
{
      ushort EAS_PID;
      uchar table_id;
      ushort section_lenth;
      ushort table_id_extention;
      uchar sequence_number;
      	uchar next_indicator;
	uchar section_number;
	uchar last_section_number;
      uchar protocol_version;
      ushort EAS_event_ID;
      uchar EAS_originator_code[4];
      uchar EAS_event_code_length;
      uchar EAS_event_code[256];
      uchar nature_of_activation_text_length;
      uchar nature_of_activation_text[256];
      uchar alert_message_time_remaining;
      uint  event_start_time;
      ushort event_duration;
      uchar alert_priority;
      ushort details_OOB_source_ID;
      ushort details_major_channel_number;
      ushort details_minor_channel_number;
      ushort audio_OOB_source_ID;
      ushort alert_text_length;
      uchar alert_text[65536];
      uchar location_code_count;
      LOCATION_CODE location_code_data[256];
      uchar exception_count;
      EXCEPTION exception_data[256];
      ushort descriptors_length;    //outer descriptor
	uint CRC;
}  SCTE_EAS;

#endif
