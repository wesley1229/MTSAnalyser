/********************************************
*      MPEG-2 Transport Stream PSI analyzer *
*      copyright © Wesley Chen. all rights reserved 2013-2020 *
*      Software Verion 1.1.0           *
********************************************/

#ifndef __CODE2STRING__
#define __CODE2STRING__

//PSHEADER
uchar *transport_scramble_control[4] = {"Not scrambled",
                                                     "User-defined", 
                                                     "User-defined",
                                                     "User-defined"};
//PMT
uchar *stream_type[20] = {"ITU-T | ISO/IEC Reserved",
                                    "MPEG-1 Video", 
                                    "MPEG-2 Video",
                                    "MPEG-1 Audio",
                                    "MPEG-2 Audio",
                                    "private_sections",
                                    "PES packets containing private data",
                                    "MHEG",
                                    "DSM-CC",
                                    "ITU-T Rec. H.222.1",
                                    "DSMCC U-N message/type A",
                                    "DSMCC U-N message/type B",
                                    "DSMCC U-N message/type C",
                                    "DSMCC U-N message/type D",
                                    "MPEG-2 auxiliary",
                                    "Advance audio(AAC)",
                                    "Visual",
                                    "Audio with the LATM transport syntax",
                                    "SL-packetized stream",
                                    "Synchronized Download Protocol"};
// CDS
uchar *spacing_unit[2] = {"10 kHz spacing",
                                    "125 kHz spacing"};
                                    
uchar *Frequency_unit[2] = {"10 kHz units",
                                        "125 kHz units"};

//MMS
uchar *transmission_system[16] = {"unknown",
                                              "Reserved (ETSI)",
                                              "ITU-T annex B",
                                              "Defined for use in other systems",
                                              "ATSC",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)",
                                              "Reserved (satellite)"};

uchar *inner_coding_mode[16] = {"rate 5/11 coding",
                                              "rate 1/2 coding",
                                              "Reserved",
                                              "rate 3/5 coding",
                                              "Reserved",
                                              "rate 2/3 coding",
                                              "Reserved",
                                              "rate 3/4 coding",
                                              "rate 4/5 coding",
                                              "rate 5/6 coding",
                                              "Reserved",
                                              "rate 7/8 coding",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved",
                                              "none"};

uchar *modulation_format[32] = {"unknown",
                                              "QPSK",
                                              "BPSK",
                                              "OQPSK",
                                              "VSB 8",
                                              "VSB 16",
                                              "QAM 16",
                                              "QAM 32",
                                              "QAM 64",
                                              "QAM 80",
                                              "QAM 96",
                                              "QAM 112",
                                              "QAM 128",
                                              "QAM 160",
                                              "QAM 192",
                                              "QAM 224",
                                              "QAM 256",
                                              "QAM 320",
                                              "QAM 384",                                              
                                              "QAM 448",
                                              "QAM 512",
                                              "QAM 640",
                                              "QAM 768",
                                              "QAM 896",
                                              "QAM 1024",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved",
                                              "Reserved"};

//VCM                                              
uchar *path_select[2] = {"path 1",
                                   "path 2"};                                  

uchar *transport_type[2] = {"MPEG-2 transport",
                                       "non-MPEG-2 transport"};     

uchar *channel_type[16] = {"normal",
                                     "hidden",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved"};

uchar *video_standard[16] = {"NTSC",
                                     "PAL 625",
                                     "PAL 525",
                                     "SECAM",
                                     "MAC",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved"};

uchar *alignment_type[5] = {"Reserved",
                                     "Slice, or video access unit",
                                     "Video access unit",
                                     "GOP, or SEQ",
                                     "SEQ"};

uchar *audio_type[4] = {"Undefined",
                                     "Clean effects",
                                     "Hearing impaired",
                                     "Visual impaired commentary"};
                           
#endif
