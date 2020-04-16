/********************************************
*      MPEG-2 Transport Stream PSI analyzer *
*      copyright © Wesley Chen. all rights reserved 2013-2020 *
*      Software Verion 1.1.0           *
********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MTSAnalyser.h"
#include "code2string.h"

static bool PAT_Table_ready = false;
static bool PMT_Table_ready = false;
static bool CAT_Table_ready = false;
static bool TSDT_Table_ready = false;
static bool CDS_Table_ready = false;
static bool MMS_Table_ready = false;
static bool SNS_Table_ready = false;
static bool STT_Table_ready = false;
static bool DCM_Table_ready = false;
static bool VCM_Table_ready = false;
static bool ICM_Table_ready = false;
static bool INBAND_EAS_Table_ready = false;
static bool OOB_EAS_Table_ready = false;

/* PMT list head node */
static TSPK_PMT *ex_PMT_list = NULL;
static ushort ReSem_buf_oob_ptr = 0;
static ushort ReSem_buf_eas_ptr = 0;
static bool CDS_overflow = false;
static bool MMS_overflow = false;
static bool SNS_overflow = false;
static bool VCM_overflow = false;
static bool DCM_overflow = false;
static bool ICM_overflow = false;
static bool STT_overflow = false;
static bool INBAND_EAS_overflow = false;
static bool OOB_EAS_overflow = false;
static bool VCT_detect_lock = false;
static ushort section_len;
static ushort section_num;
static ushort section_count;
ushort copy_len_oob;
ushort copy_len_eas;

/*********************************
*           MPEG-2 standard function           *
*********************************/

int mpeg2_descriptor_parser(uchar *mpeg2_payload, int descriptor_count)
{
    uchar *mpeg2_payload_start = mpeg2_payload;
    int i=0;
    int j=0;
    int k=0;
    int x=0;
    int descriptor_num = descriptor_count;
    int substart_byte = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    uchar iso639_descriptor_len;
    bool MPEG_1_only_flag;
    bool cc_type;
    uchar byte_shift = 0;
    while(descriptor_num > 0)
    {
        descriptor_tag = mpeg2_payload_start[substart_byte];
        descriptor_len =  mpeg2_payload_start[substart_byte+1];

        switch(descriptor_tag)
        {
            case 2:
                    printf("        %d)descriptor tag (video_stream_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)multiple_frame_rate_flag (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 7));
                    printf("        %d)frame_rate_code (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 3) & 0xf);
                    printf("        %d)MPEG_1_only_flag (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 2) & 0x1);
                    printf("        %d)constrained_parameter_flag (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 1) & 0x1);
                    printf("        %d)still_picture_flag (%d)\n", j+1, mpeg2_payload_start[substart_byte+2] & 0x1);
                    MPEG_1_only_flag = (mpeg2_payload_start[substart_byte+2] >> 2) & 0x1;
                    if(MPEG_1_only_flag == 0)
                    {
                        printf("        %d)profile_and_level_indication (%d)\n", j+1, mpeg2_payload_start[substart_byte+3]);
                        printf("        %d)chroma_format (0x%x)\n", j+1, mpeg2_payload_start[substart_byte+4] >> 6);
                        printf("        %d)frame_rate_extension_flag (%d)\n", j+1, mpeg2_payload_start[substart_byte+4] >> 5);
                    }
                    j++;
                    break;
            case 3:
                    printf("        %d)descriptor tag (audio_stream_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)free_format_flag (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 7));
                    printf("        %d)ID (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 6) & 0x1);
                    printf("        %d)layer (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 4) & 0x3);
                    printf("        %d)variable_rate_audio_indicator (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 3) & 0x1);
                    j++;
                    break;
            case 4:
                    printf("        %d)descriptor tag (hierarchy_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)hierarchy_type (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] & 0xf));
                    printf("        %d)hierarchy_layer_index (%d)\n", j+1, (mpeg2_payload_start[substart_byte+3] & 0x3f) );
                    printf("        %d)hierarchy_embedded_layer_index (%d)\n", j+1, (mpeg2_payload_start[substart_byte+4] & 0x3f));
                    printf("        %d)hierarchy_channel (%d)\n", j+1, (mpeg2_payload_start[substart_byte+4] & 0x3f));
                    j++;
                    break;
            case 5:
                    printf("        %d)descriptor tag (registration_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)format_identifier (0x%x)\n", j+1, ( (mpeg2_payload_start[substart_byte+2] << 24) | (mpeg2_payload_start[substart_byte+3] << 16) | (mpeg2_payload_start[substart_byte+4] << 8) | (mpeg2_payload_start[substart_byte+5])) );
                    if(descriptor_len > 4)
                    {
                        printf("        %d)additional_identification_info>\n                          ", j+1);
                        for(i=0; i< (descriptor_len-4); i++)
                        {
                            printf("(0x%x)", mpeg2_payload_start[substart_byte+6+i]);
                        }
                        printf("\n");
                    }
                    j++;
                    break;
            case 6:    
                    printf("        %d)descriptor tag (data_stream_alignment_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    if(mpeg2_payload_start[substart_byte+2] < 5)
                        printf("        %d)alignment_type (%d)/(%s)\n", j+1, mpeg2_payload_start[substart_byte+2], alignment_type[mpeg2_payload_start[substart_byte+2]]);
                    else
                        printf("        %d)alignment_type (%d)/(Reserved)\n", j+1, mpeg2_payload_start[substart_byte+2]);
                    j++;
                    break;
            case 7:    
                    printf("        %d)descriptor tag (target_background_grid_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)horizontal_size (%d)\n", j+1, ((mpeg2_payload_start[substart_byte+2] << 8) | mpeg2_payload_start[substart_byte+3]) >> 2  );
                    printf("        %d)vertical_size (%d)\n", j+1, (((mpeg2_payload_start[substart_byte+3] & 0x3) << 12) | (((mpeg2_payload_start[substart_byte+4] << 8) | mpeg2_payload_start[substart_byte+5] ) >> 4)));
                    printf("        %d)aspect_ratio_information (%d)\n", j+1, (mpeg2_payload_start[substart_byte+5] & 0xf ) );
                    j++;
                    break;
            case 8:    
                    printf("        %d)descriptor tag (video_window_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)horizontal_offset (%d)\n", j+1, ((mpeg2_payload_start[substart_byte+2] << 8) | mpeg2_payload_start[substart_byte+3]) >> 2  );
                    printf("        %d)vertical_offset (%d)\n", j+1, (((mpeg2_payload_start[substart_byte+3] & 0x3) << 12) | (((mpeg2_payload_start[substart_byte+4] << 8) | mpeg2_payload_start[substart_byte+5] ) >> 4)));
                    printf("        %d)window_priority (%d)\n", j+1, (mpeg2_payload_start[substart_byte+5] & 0xf ) );
                    j++;
                    break;
            case 9:
                    printf("        %d)descriptor tag (CA_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)CA_system_ID (0x%x)\n", j+1, ((mpeg2_payload_start[substart_byte+2] << 8) | mpeg2_payload_start[substart_byte+3]));
                    printf("        %d)CA_PID (0x%x)\n", j+1, ((mpeg2_payload_start[substart_byte+4] << 8) | mpeg2_payload_start[substart_byte+5]) & 0x1fff);
                    if(descriptor_len > 4)
                    {
                        printf("        %d)Private byte>\n                              ", j+1);
                        for(i=0; i< (descriptor_len-4); i++)
                        {
                            printf("(0x%x)", mpeg2_payload_start[substart_byte+6+i]);
                        }
                        printf("\n");
                    }
                    j++;
                    break;
            case 10:
                    printf("        %d)descriptor tag (ISO_639_language_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    i=0;
                    k=0;
                    iso639_descriptor_len = descriptor_len;
                    while(iso639_descriptor_len > 0)
                    {
                        //ISO_639_language_code = ((mpeg2_payload_start[substart_byte+2+k] << 16) | (mpeg2_payload_start[substart_byte+3+k] << 8) | mpeg2_payload_start[substart_byte+4+k]);
                        printf("            %d)ISO_639_language_code ( ", i+1);
                        for (x=0; x<3; x++)
                	          printf("%c", mpeg2_payload_start[substart_byte+2+x+k]);
                	     printf(" )\n");
                	     
                        if(mpeg2_payload_start[substart_byte+5+k] < 4)
                            printf("            %d)audio_type (%s)\n", i+1, audio_type[mpeg2_payload_start[substart_byte+5+k]] );
                        else
                            printf("            %d)audio_type (Reserved)\n", i+1);
                        iso639_descriptor_len -= 4;
                        k +=4;
                        i++;
                    }
                    j++;
                    break;
            case 11:
                    printf("        %d)descriptor tag (system_clock_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)external_clock_reference_indicator (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 7));
                    printf("        %d)clock_accuracy_integer (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] & 0x3f) );
                    printf("        %d)clock_accuracy_exponent (%d)\n", j+1, (mpeg2_payload_start[substart_byte+3] >> 5));
                    j++;
                    break;
            case 12:
                    printf("        %d)descriptor tag (multiplex_buffer_utilization_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)bound_valid_flag (%d)\n", j+1, (mpeg2_payload_start[substart_byte+2] >> 7));
                    printf("        %d)LTW_offset_lower_bound (%d)\n", j+1, (((mpeg2_payload_start[substart_byte+2] & 0x7f) << 8) | mpeg2_payload_start[substart_byte+3]) );
                    printf("        %d)LTW_offset_upper_bound (%d)\n", j+1, (((mpeg2_payload_start[substart_byte+4] & 0x7f) << 8) | mpeg2_payload_start[substart_byte+5]) );
                    j++;
                    break;
            case 13:
                    printf("        %d)descriptor tag (copyright_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)copyright_identifier (0x%x)\n", j+1, ( (mpeg2_payload_start[substart_byte+2] << 24) | (mpeg2_payload_start[substart_byte+3] << 16) | (mpeg2_payload_start[substart_byte+4] << 8) | (mpeg2_payload_start[substart_byte+5])) );
                    if(descriptor_len > 4)
                    {
                        printf("        %d)additional_copyright_info>\n                          ", j+1);                    
                        for(i=0; i< (descriptor_len-4); i++)
                        {
                            printf("(0x%x)", mpeg2_payload_start[substart_byte+6+i]);
                        }
                        printf("\n");
                    }
                    j++;
                    break;  
            case 14:    
                    printf("        %d)descriptor tag (maximum_bitrate_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)maximum_bitrate (%d) byte/sec\n", j+1, ( ((mpeg2_payload_start[substart_byte+2] & 0x3f) << 16) | (mpeg2_payload_start[substart_byte+3] << 8) | mpeg2_payload_start[substart_byte+4] ) * 50);
                    j++;
                    break;
            case 15:    
                    printf("        %d)descriptor tag (private_data_indicator_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)private_data_indicator (%d)\n", j+1, ( (mpeg2_payload_start[substart_byte+2] << 24) | (mpeg2_payload_start[substart_byte+3] << 16) | (mpeg2_payload_start[substart_byte+4] << 8) | mpeg2_payload_start[substart_byte+5]));
                    j++;
                    break;
            case 16:    
                    printf("        %d)descriptor tag (smoothing_buffer_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)sb_leak_rate (%d) bit/s\n", j+1, ( ( (mpeg2_payload_start[substart_byte+2] & 0x3f) << 16) | (mpeg2_payload_start[substart_byte+3] << 8) | mpeg2_payload_start[substart_byte+4]) * 400);
                    printf("        %d)sb_size (%d) byte\n", j+1, ( ( (mpeg2_payload_start[substart_byte+5] & 0x3f) << 16) | (mpeg2_payload_start[substart_byte+6] << 8) | mpeg2_payload_start[substart_byte+7]));
                    j++;
                    break; 
            case 17:    
                    printf("        %d)descriptor tag (STD_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)leak_valid_flag (0x%x)\n", j+1,  (mpeg2_payload_start[substart_byte+2] & 0x1) );
                    j++;
                    break;
            case 18:    
                    printf("        %d)descriptor tag (ibp_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)closed_gop_flag (0x%x)\n", j+1,  (mpeg2_payload_start[substart_byte+2] >> 7) );
                    printf("        %d)identical_gop_flag (0x%x)\n", j+1,  (mpeg2_payload_start[substart_byte+2] >> 6) );
                    printf("        %d)max_gop-length (0x%x)\n", j+1,  ((mpeg2_payload_start[substart_byte+2] & 0x3f) << 8) | mpeg2_payload_start[substart_byte+3] );
                    j++;
                    break;
            case 27:    
                    printf("        %d)descriptor tag (MPEG-4_video_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)MPEG-4_visual_profile_and_level (0x%x)\n", j+1,  mpeg2_payload_start[substart_byte+2] );
                    j++;
                    break;
            case 28:    
                    printf("        %d)descriptor tag (MPEG-4_audio_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)MPEG-4_audio_profile_and_level (0x%x)\n", j+1,  mpeg2_payload_start[substart_byte+2] );
                    j++;
                    break;
            case 29:    
                    printf("        %d)descriptor tag (IOD_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)Scope_of_IOD_label (0x%x)\n", j+1,  mpeg2_payload_start[substart_byte+2] );
                    printf("        %d)IOD_label (0x%x)\n", j+1,  mpeg2_payload_start[substart_byte+3] );
                    printf("        %d)InitialObjectDescriptor (0x%x)\n", j+1,  mpeg2_payload_start[substart_byte+4] );
                    j++;
                    break;
            case 30:    
                    printf("        %d)descriptor tag (SL_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)ES_ID (0x%x)\n", j+1,  (mpeg2_payload_start[substart_byte+2] << 8) | mpeg2_payload_start[substart_byte+3] );
                    j++;
                    break;
            case 31:    
                    printf("        %d)descriptor tag (FMC_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    if(descriptor_len > 0)
                    {
                        x = 0;
                        for(i=0; i< descriptor_len; i+=3)
                        {
                            printf("            %d)ES_ID (0x%x) \n", x+1, (mpeg2_payload_start[substart_byte+2+i] << 8) | mpeg2_payload_start[substart_byte+3+i]);
                            printf("            %d)FlexMuxChannel (0x%x) \n", x+1, mpeg2_payload_start[substart_byte+4+i] );
                            x++;
                        }
                    }
                    j++;
                    break;
            case 32:    
                    printf("        %d)descriptor tag (External_ES_ID_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)External_ES_ID (0x%x)\n", j+1,  (mpeg2_payload_start[substart_byte+2] << 8) |  mpeg2_payload_start[substart_byte+3]);
                    j++;
                    break;
            case 33:    
                    printf("        %d)descriptor tag (Muxcode_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    if(descriptor_len > 0)
                    {
                        printf("        %d)MuxCodeTableEntry>\n                          ", j+1);
                        x = 0;
                        for(i=0; i< (descriptor_len); i++)
                        {
                            printf("(0x%x)", mpeg2_payload_start[substart_byte+2+i]);
                        }
                        printf("\n");
                    }
                    j++;
                    break;
            case 34:    
                    printf("        %d)descriptor tag (FmxBufferSize_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    j++;
                    break;
            case 35:    
                    printf("        %d)descriptor tag (MultiplexBuffer_descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)MB_buffer_size (%d)\n", j+1,  (mpeg2_payload_start[substart_byte+2] << 16) |  (mpeg2_payload_start[substart_byte+3] << 8) | mpeg2_payload_start[substart_byte+4] );
                    printf("        %d)TB_leak_rate (%d)\n", j+1,  (mpeg2_payload_start[substart_byte+5] << 16) |  (mpeg2_payload_start[substart_byte+6] << 8) | mpeg2_payload_start[substart_byte+7] );
                    j++;
                    break;
            case 0x80:
                    printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)(detail info not yet implement)\n", j+1);
                    j++;
                    break;
            case 0x86:
                    printf("        %d)descriptor tag (Caption service descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    //printf("        %d)(detail info not yet implement)\n", j+1);
                    if((mpeg2_payload_start[substart_byte+2] & 0x1f) > 0)
                    {
                        for(i=0; i< (mpeg2_payload_start[substart_byte+2] & 0x1f); i++)
                        {
                            printf("            %d)language (", i+1);
                            for(x=0;x<3;x++)    
                                printf("%c", mpeg2_payload_start[substart_byte+3+x+byte_shift]);
                            printf(")\n");
                            cc_type = mpeg2_payload_start[substart_byte+6+byte_shift] >> 7;
                            printf("            %d)cc_type (%d)\n", i+1, cc_type);
                            if(!cc_type)
                            {
                                //line21
                                if((mpeg2_payload_start[substart_byte+6+byte_shift] & 0x1))
                                    printf("            %d)line21_field (Field 2 of the NTSC waveform)\n", i+1);
                                else
                                    printf("            %d)line21_field (Field 1 of the NTSC waveform)\n", i+1);
                            }
                            else
                            {
                                //caption service number
                                printf("            %d)caption_service_number (%d)\n", i+1, (mpeg2_payload_start[substart_byte+6+byte_shift] & 0x3f));
                            }
                            printf("            %d)easy_reader (%d)\n", i+1, (mpeg2_payload_start[substart_byte+7+byte_shift] >> 7));
                            if( ((mpeg2_payload_start[substart_byte+7+byte_shift] >> 6) & 0x1))
                                printf("            %d)wide_aspect_ratio (16:9)\n", i+1);
                            else
                                printf("            %d)wide_aspect_ratio (4:3)\n", i+1);
                            byte_shift += 6;
                        }
                    }
                    j++;
                    break;
            case 0x87:
                    printf("        %d)descriptor tag (Content advisory descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)private data>\n                              ", j+1);                    
                    for(i=0; i< descriptor_len; i++)
                    {
                        printf("(0x%x)", mpeg2_payload_start[substart_byte+2+i]);
                    }
                    printf("\n");
                    j++;
                    break;
            case 0xA3:
                    printf("        %d)descriptor tag (Component name descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)(detail info not yet implement)\n", j+1);
                    j++;
                    break;
            default:
                if(descriptor_tag > 18 && descriptor_tag < 27)
                {
                    printf("        %d)descriptor tag (Defined in ISO/IEC 13818-6)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)private data>\n                              ", j+1);                    
                    for(i=0; i< descriptor_len; i++)
                    {
                        printf("(0x%x)", mpeg2_payload_start[substart_byte+2+i]);
                    }
                    printf("\n");
                    j++;
                    break;
                }
                if(descriptor_tag> 35 && descriptor_tag < 64)
                {
                    printf("        %d)descriptor tag (ISO/IEC 13818-1 Reserved)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)private data>\n                              ", j+1);                    
                    for(i=0; i< descriptor_len; i++)
                    {
                        printf("(0x%x)", mpeg2_payload_start[substart_byte+2+i]);
                    }
                    printf("\n");
                    j++;
                    break;
                }
                if(descriptor_tag> 63 && descriptor_tag < 256)
                {
                    printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)private data>\n                              ", j+1);                    
                    for(i=0; i< descriptor_len; i++)
                    {
                        printf("(0x%x)", mpeg2_payload_start[substart_byte+2+i]);
                    }
                    printf("\n");
                    j++;
                }
                break;
        }
        substart_byte = (substart_byte + descriptor_len + 2);
        descriptor_num = descriptor_num - descriptor_len - 2;
    }
    return substart_byte;
}

TSPK_PMT *TSPMT_Parser(TSPK_PMT *pmt_list_head, uchar *PMT_payload, ushort pmt_pid)
{
    uchar *PMT_payload_start = PMT_payload;
    uchar ver_temp;
    ushort program_num;
    int i = 0;
    int j = 0;
    int ES_Num = 0;
    int last_4num = 7;
    int ES_start_len = 12;
    int ES_info_gate = 0;
    if(PMT_payload == NULL)
    {
        perror("PMT input data error\n");
        return pmt_list_head;
    }
    TSPK_PMT *pmt_list = (TSPK_PMT *)malloc(sizeof(TSPK_PMT));
    pmt_list->PMT_PID = pmt_pid;
    pmt_list->table_id = PMT_payload_start[0];
    pmt_list->section_lenth = ( PMT_payload[1] << 8 | PMT_payload[2]) & SECTION_LEN_MASK;
    pmt_list->program_number = ( PMT_payload[3] << 8 | PMT_payload[4]);
    ver_temp = PMT_payload[5];
    pmt_list->version_number = ( (ver_temp >> 1) & VERSION_MASK);
    pmt_list->next_indicator = ver_temp  & CURRENT_NEXT_MASK;
    if(pmt_list->next_indicator != 1)
    {
        perror("Not valid PMT\n");
        free(pmt_list);
        return pmt_list_head;
    }
    if(pmt_list->section_lenth < 13)
    {
        perror("PMT section length format error\n");
        free(pmt_list);
        return pmt_list_head;
    }
    pmt_list->section_number = PMT_payload[6];
    pmt_list->last_section_number = PMT_payload[7];
        if(pmt_list->last_section_number > 0)
    {
        perror("PMT section number is over 1, we don't support it \n");
        free(pmt_list);
        return pmt_list_head;
    }
    pmt_list->PCR_PID = ( PMT_payload[8] << 8 | PMT_payload[9]) & PCR_PID_MASK;
    pmt_list->programInfo_len = ( PMT_payload[10] << 8 | PMT_payload[11]) & PROGRAM_INFO_MASK;
    pmt_list->ES_Num = 0;
    printf("==============PMT===============\n");
    printf("PMT_PID (0x%x) \n",pmt_list->PMT_PID);
    printf("table_id (0x%x) \n",pmt_list->table_id);
    printf("section_lenth (%d) \n",pmt_list->section_lenth);
    printf("program_number (%d) \n",pmt_list->program_number);
    printf("version_number (%d) \n",pmt_list->version_number);
    printf("current_next_indicator (%d) \n",pmt_list->next_indicator);
    printf("section_number (%d) \n",pmt_list->section_number);
    printf("last_section_number (%d) \n",pmt_list->last_section_number);
    printf("PCR_PID (0x%x) \n",pmt_list->PCR_PID);
    printf("programInfo_len (%d) \n",pmt_list->programInfo_len);
    mpeg2_descriptor_parser(&PMT_payload[12], pmt_list->programInfo_len);
    ES_start_len = pmt_list->programInfo_len+12;
    //reserve for descriptor
    ES_Num = pmt_list->section_lenth - 13 - pmt_list->programInfo_len;
    printf("ES>\n");
    while(ES_Num>0)
    {
        pmt_list->es_data[i].stream_type = PMT_payload[ES_start_len+5*j+ES_info_gate];
        pmt_list->es_data[i].ES_PID = ( (PMT_payload[ES_start_len+1+5*j+ES_info_gate] << 8) | PMT_payload[ES_start_len+2+5*j+ES_info_gate]) & ES_PID_MASK;
        pmt_list->es_data[i].ES_INFO_LEN = ( (PMT_payload[ES_start_len+3+5*j+ES_info_gate] << 8) | PMT_payload[ES_start_len+4+5*j+ES_info_gate]) & ES_LEN_MASK;
        if(pmt_list->es_data[i].stream_type < 0x15)
            printf("    %d)stream_type (0x%x)/(%s)", pmt_list->ES_Num+1, pmt_list->es_data[i].stream_type, stream_type[pmt_list->es_data[i].stream_type]);    
        else if(pmt_list->es_data[i].stream_type > 0x14 && pmt_list->es_data[i].stream_type< 0x7f)
            printf("    %d)stream_type (0x%x)/(MPEG-2 Reserved)", pmt_list->ES_Num+1, pmt_list->es_data[i].stream_type);    
        else
            printf("    %d)stream_type (0x%x)/(User Private)", pmt_list->ES_Num+1, pmt_list->es_data[i].stream_type);    
        printf("         ES_PID  (0x%x)", pmt_list->es_data[i].ES_PID);
        printf("         ES_DESCRIPTOR_LEN  (%d)\n", pmt_list->es_data[i].ES_INFO_LEN);
        //reserve for descriptor
        mpeg2_descriptor_parser(&PMT_payload[ES_start_len+4+5*j+ES_info_gate+1], pmt_list->es_data[i].ES_INFO_LEN);
        ES_info_gate += pmt_list->es_data[i].ES_INFO_LEN;
        last_4num = ES_start_len+4+5*j+ES_info_gate;
        ES_Num = ES_Num - pmt_list->es_data[i].ES_INFO_LEN - 5;
        pmt_list->ES_Num++;
        i++;j++;
    }
    printf("ES Numbers (%d) \n", pmt_list->ES_Num);
    pmt_list->CRC = (PMT_payload[last_4num+1] << 24) | (PMT_payload[last_4num+2] << 16 ) | (PMT_payload[last_4num+3] << 8) | PMT_payload[last_4num+4];
    printf("CRC32 (0x%x) \n", pmt_list->CRC);
    printf("================================\n"); 
    pmt_list->next = pmt_list_head;
    pmt_list_head = pmt_list;
    return pmt_list_head;

}

int TSPAT_Parser(TSPK_PAT *pat_data, uchar *PAT_payload)
{
    uchar *PAT_payload_start = PAT_payload;
    uchar ver_temp;
    ushort program_num;
    int i = 0;
    int j = 0;
    int PMT_Num = 0;
    int last_4num = 7;
    if(pat_data == NULL || PAT_payload == NULL)
    {
        perror("PAT input data error\n");
        return 1;
    }
    pat_data->table_id = PAT_payload_start[0];
    pat_data->section_lenth = ( PAT_payload_start[1] << 8 | PAT_payload_start[2]) & SECTION_LEN_MASK;
    if(pat_data->section_lenth < 9)
    {
        perror("PAT section length format error\n");
        return 1;
    }
    pat_data->stream_id = (PAT_payload_start[3] << 8) | PAT_payload_start[4];
    ver_temp = PAT_payload_start[5];
    pat_data->version_number = ( (ver_temp >> 1) & VERSION_MASK);
    pat_data->next_indicator = ver_temp  & CURRENT_NEXT_MASK;
    if(pat_data->next_indicator != 1)
    {
        perror("Not valid PAT\n");
        return 1;
    }
    pat_data->section_number = PAT_payload_start[6];
    pat_data->last_section_number = PAT_payload_start[7];
    
    if(pat_data->last_section_number > 0)
    {
        perror("PAT section number is over 1, we don't support it \n");
        return 1;
    }

    PMT_Num = (pat_data->section_lenth - 9)/4;
    pat_data->PMT_Num = PMT_Num;
        
    while(j<PMT_Num)
    {
        program_num = (PAT_payload[8+4*j] << 8 | PAT_payload[9+4*j]);
        if(program_num == 0)
        {
            //Network PID
            pat_data->PMT_Num--;
            j++;
            continue;
        }
        pat_data->program_number[i] = program_num;
        pat_data->PMT_PID[i] = ( (PAT_payload[10+4*j] << 8) | PAT_payload[11+4*j]) & PMT_PID_MASK;
        last_4num = 11+4*j;
        i++;j++;
    }
    
    pat_data->CRC = (PAT_payload_start[last_4num+1] << 24) | (PAT_payload_start[last_4num+2] << 16 ) | (PAT_payload_start[last_4num+3] << 8) | PAT_payload_start[last_4num+4];
    return 0;

}

int TSPKHead_Parser(TSPK_HEADER *ts_data, uint ts_header)
{
    //printf("%x #\n", ts_header);
    if(ts_header == 0 || ts_data == NULL)
    {
        perror("TS header input data error\n");
        return 1;
    }
    ts_data->error_bit = (ts_header >> 23) & TRANSPORT_BIT_MASK;
    ts_data->playload_start_bit = (ts_header >> 22) & PAYLOAD_BIT_MASK;
    ts_data->priority_bit = (ts_header >> 21) & PRIORITY_BIT_MASK;
    ts_data->PID = (ts_header >> 8) & PID_MASK;
    ts_data->scramble_control = (ts_header >> 6) & SCRAMBLE_BIT_MASK;
    ts_data->adaption_field_control = (ts_header >> 4) & ADAPTATION_FIELD_MASK;
    ts_data->continuity_counter = ts_header & CONTINUITY_CONNTER_MASK;
    return 0;
}

int TSCAT_Parser(TSPK_CAT *cat_data, uchar *CAT_payload)
{
    uchar *CAT_payload_start = CAT_payload;
    uchar ver_temp;
    int substart_byte = 8;
    int descriptor_num = 0;
    int crc_start = 0;
    int i=0;
    int j=0;
    uchar descriptor_tag;
    uchar descriptor_len;
    if(cat_data == NULL || CAT_payload == NULL)
    {
        perror("CAT input data error\n");
        return 1;
    }
    cat_data->table_id = CAT_payload_start[0];
    cat_data->section_lenth = ( CAT_payload_start[1] << 8 | CAT_payload_start[2]) & SECTION_LEN_MASK;
    if(cat_data->section_lenth < 9)
    {
        perror("CAT section length format error\n");
        return 1;
    }
    ver_temp = CAT_payload_start[5];
    cat_data->version_number = ( (ver_temp >> 1) & VERSION_MASK);
    cat_data->next_indicator = ver_temp  & CURRENT_NEXT_MASK;
    if(cat_data->next_indicator != 1)
    {
        perror("Not valid CAT\n");
        return 1;
    }
    cat_data->section_number = CAT_payload_start[6];
    cat_data->last_section_number = CAT_payload_start[7];
    if(cat_data->last_section_number > 0)
    {
        perror("CAT section number is over 1, we don't support it \n");
        return 1;
    }
    printf("==============CAT===============\n");
    printf("CAT_PID (0x1) \n");
    printf("table_id (0x%x) \n",cat_data->table_id);
    printf("section_lenth (%d) \n",cat_data->section_lenth);
    printf("version_number (%d) \n",cat_data->version_number);
    printf("current_next_indicator (%d) \n",cat_data->next_indicator);
    printf("section_number (%d) \n",cat_data->section_number);
    printf("last_section_number (%d) \n",cat_data->last_section_number);
    descriptor_num = cat_data->section_lenth - 9;
    cat_data->outer_descriptor_num = descriptor_num;
    printf("outer descriptor number (%d) \n",cat_data->outer_descriptor_num);

    //substart_byte += descriptor_num;
    crc_start = substart_byte + descriptor_num;
    
    //reserve for descriptor
    substart_byte += mpeg2_descriptor_parser(&CAT_payload_start[substart_byte], descriptor_num);

    cat_data->CRC = (CAT_payload_start[crc_start] << 24) | (CAT_payload_start[crc_start+1] << 16 ) | (CAT_payload_start[crc_start+2] << 8) | CAT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", cat_data->CRC);
    //printf("outside byte (%x) (%x) \n", CAT_payload_start[crc_start+4], CAT_payload_start[crc_start+5]);
    printf("================================\n"); 
    return 0;
}

int TSDT_Parser(TSPK_TSDT *tsdt_data, uchar *TSDT_payload)
{
    uchar *TSDT_payload_start = TSDT_payload;
    uchar ver_temp;
    int substart_byte = 8;
    int descriptor_num = 0;
    int crc_start = 0;
    if(tsdt_data == NULL || TSDT_payload == NULL)
    {
        perror("TSDT input data error\n");
        return 1;
    }
    tsdt_data->table_id = TSDT_payload_start[0];
    tsdt_data->section_lenth = ( TSDT_payload_start[1] << 8 | TSDT_payload_start[2]) & SECTION_LEN_MASK;
    if(tsdt_data->section_lenth < 9)
    {
        perror("TSDT section length format error\n");
        return 1;
    }
    ver_temp = TSDT_payload_start[5];
    tsdt_data->version_number = ( (ver_temp >> 1) & VERSION_MASK);
    tsdt_data->next_indicator = ver_temp  & CURRENT_NEXT_MASK;
    if(tsdt_data->next_indicator != 1)
    {
        perror("Not valid TSDT\n");
        return 1;
    }
    tsdt_data->section_number = TSDT_payload_start[6];
    tsdt_data->last_section_number = TSDT_payload_start[7];
    
    if(tsdt_data->last_section_number > 0)
    {
        perror("TSDT section number is over 1, we don't support it \n");
        return 1;
    }

    descriptor_num = tsdt_data->section_lenth - 9;
    tsdt_data->outer_descriptor_num = descriptor_num;
    printf("==============TSDT===============\n");
    printf("TSDT_PID (0x2) \n");
    printf("table_id (0x%x) \n",tsdt_data->table_id);
    printf("section_lenth (%d) \n",tsdt_data->section_lenth);
    printf("version_number (%d) \n",tsdt_data->version_number);
    printf("current_next_indicator (%d) \n",tsdt_data->next_indicator);
    printf("section_number (%d) \n",tsdt_data->section_number);
    printf("last_section_number (%d) \n",tsdt_data->last_section_number);
    printf("outer descriptor number (%d) \n",tsdt_data->outer_descriptor_num);
    //substart_byte += descriptor_num;
    //reserver for outer descriptor
    crc_start = substart_byte + descriptor_num;
    substart_byte += mpeg2_descriptor_parser(&TSDT_payload_start[substart_byte], descriptor_num);
    
    tsdt_data->CRC = (TSDT_payload_start[crc_start] << 24) | (TSDT_payload_start[crc_start+1] << 16 ) | (TSDT_payload_start[crc_start+2] << 8) | TSDT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", tsdt_data->CRC);
    printf("================================\n"); 
    return 0;
}

/*********************************
*             SCTE 65 main function              *
*********************************/

int SCTE65_descriptor_Parser(uchar *scte65_payload, int descriptor_count)
{
    uchar *scte65_payload_start = scte65_payload;
    int i=0;
    int j=0;
    int descriptor_num = descriptor_count;
    int substart_byte = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    while(descriptor_num > 0)
    {
         descriptor_tag = scte65_payload_start[substart_byte];
         descriptor_len = scte65_payload_start[substart_byte+1];
         switch(descriptor_tag)
         {
             case 0x80:
                     printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     printf("        %d)(detail info not yet implement)\n", j+1);
                     j++;
                     break;
             case 0x93:
                     printf("        %d)descriptor tag (Revision Detection Descriptor)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     printf("        %d)table_version_number (%d)\n", j+1, scte65_payload_start[substart_byte+2] & 0x1f);
                     printf("        %d)section_number(%d)\n", j+1, scte65_payload_start[substart_byte+3]);
                     printf("        %d)last_section_number(%d)\n", j+1, scte65_payload_start[substart_byte+4]);
                     j++;
                     break;
             case 0x94:
                     printf("        %d)descriptor tag (Two-part Channel Number Descriptor)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     printf("        %d)major_channel_number (%d)\n", j+1, (scte65_payload_start[substart_byte+2] << 8 | scte65_payload_start[substart_byte+3]) & 0x3ff);
                     printf("        %d)minor_channel_number(%d)\n", j+1, (scte65_payload_start[substart_byte+4] << 8 | scte65_payload_start[substart_byte+5]) & 0x3ff);
                     j++;
                     break;
             case 0x95:
                     printf("        %d)descriptor tag (channel_properties_descriptor)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     printf("        %d)channel_TSID (%d)\n", j+1, (scte65_payload_start[substart_byte+2] << 8 | scte65_payload_start[substart_byte+3]));
                     printf("        %d)out_of_band_channel (%d)\n", j+1, (scte65_payload_start[substart_byte+4] >> 1) & 0x1);
                     printf("        %d)access_controlled (%d)\n", j+1, scte65_payload_start[substart_byte+4] & 0x1);
                     printf("        %d)hide_guide (%d)\n", j+1, (scte65_payload_start[substart_byte+5] >> 7) & 0x1);
                     printf("        %d)service_type (%d)\n", j+1, scte65_payload_start[substart_byte+5] & 0x3f);
                     j++;
                     break;
             case 0x96:
                     printf("        %d)descriptor tag (channel_properties_descriptor)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     if( (scte65_payload_start[substart_byte+2] >> 7) )
                        printf("        %d)DS_status (In daylight savings time)\n", j+1);
                     else
                        printf("        %d)DS_status (Not in daylight savings time)\n", j+1);
                     printf("        %d)DS_day_of_month (%d)\n", j+1, (scte65_payload_start[substart_byte+2] & 0x1f) );
                     printf("        %d)DS_hour (%d)\n", j+1, scte65_payload_start[substart_byte+3]);
                     j++;
                     break;                     
             default:
                 if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                 {
                     printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                     printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                     printf("        %d)private data>\n                              ", j+1);                    
                     for(i=0; i< descriptor_len; i++)
                     {
                        printf("(0x%x)", scte65_payload_start[substart_byte+2+i]);
                     }
                     printf("\n");
                     j++;
                 }
                 break;
         }
         descriptor_num = descriptor_num - descriptor_len - 2;  
         substart_byte = substart_byte + descriptor_len + 2;
    }
    return substart_byte;
}

int NIT_CDS_Parser(SCTE_NIT_CDS *nit_cds_data, uchar *NIT_payload, ushort NIT_pid)
{
    uchar *NIT_payload_start = NIT_payload;
    int i = 0;
    int j = 0;
    int k = 0;
    int CDS_Num = 0;
    int substart_byte = 7;
    int descriptor_num = 0;
    int crc_start = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    if(NIT_payload == NULL || nit_cds_data == NULL)
    {
        perror("NIT-CDS input data error\n");
        return 1;
    }
    nit_cds_data->NIT_PID = NIT_pid;
    nit_cds_data->table_id = NIT_payload_start[0];
    nit_cds_data->section_lenth = (NIT_payload_start[1] << 8 | NIT_payload_start[2]) & SECTION_LEN_MASK;
    nit_cds_data->protocol_version =   NIT_payload_start[3] & PROTOCOL_MASK;
    nit_cds_data->first_index = NIT_payload_start[4];
    nit_cds_data->number_of_records = NIT_payload_start[5];
    nit_cds_data->transmission_medium = NIT_payload_start[6] >> 4;
    nit_cds_data->table_subtype = NIT_payload_start[6] & SUBTYPE_MASK;
    
    if(nit_cds_data->first_index == 0)
    {
        perror("NIT-CDS error first index\n");
        return 1;
    }
    if(nit_cds_data->protocol_version != 0x0)
    {
        perror("NIT-CDS PROTOCOL version error!!!\n");
        return 1;
    }
    if(nit_cds_data->section_lenth < 8 || nit_cds_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("NIT-CDS section length format error\n");
        return 1;
    }
    crc_start = 3 + nit_cds_data->section_lenth - 4;
    descriptor_num = nit_cds_data->section_lenth - 8;
    printf("=============NIT-CDS============\n");
    printf("NIT_PID (0x%x) \n",nit_cds_data->NIT_PID);
    printf("table_id (0x%x) \n",nit_cds_data->table_id);
    printf("section_lenth (%d) \n",nit_cds_data->section_lenth);
    printf("protocol_version (%d) \n",nit_cds_data->protocol_version);
    printf("first_index (%d) \n",nit_cds_data->first_index);
    printf("number_of_records (%d) \n",nit_cds_data->number_of_records);
    printf("transmission_medium (0x%x) \n",nit_cds_data->transmission_medium);
    printf("table_subtype (%d) \n",nit_cds_data->table_subtype);
    printf("CDS>\n");
    for(i=0; i<nit_cds_data->number_of_records;i++)
    {
        if(nit_cds_data->table_subtype == 1)
        {
            //CDS 5 byte
            nit_cds_data->cds_data[CDS_Num].number_of_carriers = NIT_payload_start[substart_byte];
            nit_cds_data->cds_data[CDS_Num].spacing_unit = NIT_payload_start[substart_byte+1] >> 7;
            nit_cds_data->cds_data[CDS_Num].frequency_spacing = (NIT_payload_start[substart_byte+1] << 8 | NIT_payload_start[substart_byte+2]) & FREQUENCYSPACE_MASK;
            nit_cds_data->cds_data[CDS_Num].frequency_unit = NIT_payload_start[substart_byte+3] >> 7;
            nit_cds_data->cds_data[CDS_Num].first_carrier_frquency = (NIT_payload_start[substart_byte+3] << 8 | NIT_payload_start[substart_byte+4]) & FIRST_CARRIER_FREQUENCY_MASK;
            printf("    %d)number_of_carriers (%d) \n", CDS_Num+1, nit_cds_data->cds_data[CDS_Num].number_of_carriers);
            printf("    %d)spacing_unit (%d)/(%s) \n", CDS_Num+1, nit_cds_data->cds_data[CDS_Num].spacing_unit,spacing_unit[nit_cds_data->cds_data[CDS_Num].spacing_unit]);
            printf("    %d)frequency_spacing (%d) \n", CDS_Num+1, nit_cds_data->cds_data[CDS_Num].frequency_spacing);
            printf("    %d)frequency_unit (%d)/(%s) \n", CDS_Num+1, nit_cds_data->cds_data[CDS_Num].frequency_unit, Frequency_unit[nit_cds_data->cds_data[CDS_Num].frequency_unit]);
            printf("    %d)first_carrier_frquency (%d) \n", CDS_Num+1, nit_cds_data->cds_data[CDS_Num].first_carrier_frquency);
            CDS_Num++;
            substart_byte += 5;
            descriptor_num -= 5;
        }
        else
        {
            perror("NIT-CDS invalid sub table format\n");
            return 1;
        }
        nit_cds_data->descriptor_count = NIT_payload_start[substart_byte];
        //printf("    %d)descriptor_count (%d) \n",CDS_Num, nit_cds_data->descriptor_count);
        substart_byte += 1;
        descriptor_num -= 1;
        for(j=0; j<nit_cds_data->descriptor_count; j++)
        {
            descriptor_tag = NIT_payload_start[substart_byte];
            descriptor_len = NIT_payload_start[substart_byte+1];
            switch(descriptor_tag)
            {
                case 0x80:
                        printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        printf("        %d)(detail info not yet implement)\n", j+1);
                    break;
                case 0x93:
                        printf("        %d)descriptor tag (Revision Detection Descriptor)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        printf("        %d)table_version_number (%d)\n", j+1, NIT_payload_start[substart_byte+2] & 0x1f);
                        printf("        %d)section_number(%d)\n", j+1, NIT_payload_start[substart_byte+3]);
                        printf("        %d)last_section_number(%d)\n", j+1, NIT_payload_start[substart_byte+4]);
                    break;
                default:
                    if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                    {
                        printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        for(k=0; k< descriptor_len; k++)
                        {
                           printf("(0x%x)", NIT_payload_start[substart_byte+2+k]);
                        }
                        printf("\n");
                    }
                    break;
            }
            descriptor_num = descriptor_num - descriptor_len - 2;  
            substart_byte = substart_byte + descriptor_len + 2;
        }
    }
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    nit_cds_data->outer_descriptor_num = descriptor_num;
    nit_cds_data->CDS_Num = CDS_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&NIT_payload_start[substart_byte], descriptor_num);
    printf("CDS numbers (%d) \n", nit_cds_data->CDS_Num);
    //nit_cds_data->CRC = (NIT_payload_start[substart_byte] << 24) | (NIT_payload_start[substart_byte+1] << 16 ) | (NIT_payload_start[substart_byte+2] << 8) | NIT_payload_start[substart_byte+3];
    nit_cds_data->CRC = (NIT_payload_start[crc_start] << 24) | (NIT_payload_start[crc_start+1] << 16 ) | (NIT_payload_start[crc_start+2] << 8) | NIT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", nit_cds_data->CRC);
    //printf("outside byte (%x) (%x) \n", NIT_payload_start[crc_start+4], NIT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

int NIT_MMS_Parser(SCTE_NIT_MMS *nit_mms_data, uchar *NIT_payload, ushort NIT_pid)
{
    uchar *NIT_payload_start = NIT_payload;
    int i = 0;
    int j = 0;
    int k = 0;
    int MMS_Num = 0;
    int substart_byte = 7;
    int descriptor_num = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    int crc_start;
    if(NIT_payload == NULL || nit_mms_data == NULL)
    {
        perror("NIT-MMS input data error\n");
        return 1;
    }
    nit_mms_data->NIT_PID = NIT_pid;
    nit_mms_data->table_id = NIT_payload_start[0];
    nit_mms_data->section_lenth = ( NIT_payload_start[1] << 8 | NIT_payload_start[2]) & SECTION_LEN_MASK;
    nit_mms_data->protocol_version =   NIT_payload_start[3] & PROTOCOL_MASK;
    nit_mms_data->first_index = NIT_payload_start[4];
    nit_mms_data->number_of_records = NIT_payload_start[5];
    nit_mms_data->transmission_medium = NIT_payload_start[6] >> 4;
    nit_mms_data->table_subtype = NIT_payload_start[6] & SUBTYPE_MASK;
   
    if(nit_mms_data->first_index == 0)
    {
        perror("NIT-MMS error first index\n");
        return 1;
    }
    if(nit_mms_data->protocol_version != 0x0)
    {
        perror("NIT-MMS PROTOCOL version error!!!\n");
        return 1;
    }
    if(nit_mms_data->section_lenth < 8 || nit_mms_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("NIT-MMS section length format error\n");
        return 1;
    }
    crc_start = 3 + nit_mms_data->section_lenth - 4;
    descriptor_num = nit_mms_data->section_lenth - 8;
    printf("============NIT-MMS=============\n");
    printf("NIT_PID (0x%x) \n",nit_mms_data->NIT_PID);
    printf("table_id (0x%x) \n",nit_mms_data->table_id);
    printf("section_lenth (%d) \n",nit_mms_data->section_lenth);
    printf("protocol_version (%d) \n",nit_mms_data->protocol_version);
    printf("first_index (%d) \n",nit_mms_data->first_index);
    printf("number_of_records (%d) \n",nit_mms_data->number_of_records);
    printf("transmission_medium (0x%x) \n",nit_mms_data->transmission_medium);
    printf("table_subtype (%d) \n",nit_mms_data->table_subtype);
    printf("MMS>\n");
    for(i=0; i<nit_mms_data->number_of_records;i++)
    {
        if(nit_mms_data->table_subtype == 2)
        {
            //MMS 6 byte
            nit_mms_data->mms_data[MMS_Num].transmission_system = NIT_payload_start[substart_byte] >> 4;
            nit_mms_data->mms_data[MMS_Num].inner_coding_mode = NIT_payload_start[substart_byte] & INNER_CODE_MASK;
            nit_mms_data->mms_data[MMS_Num].split_bitstream_mode = NIT_payload_start[substart_byte+1] >> 7;
            nit_mms_data->mms_data[MMS_Num].modulation_format = NIT_payload_start[substart_byte+1] & MODULATION_MASK;
            nit_mms_data->mms_data[MMS_Num].symbol_rate = ((NIT_payload_start[substart_byte+2] << 24) | (NIT_payload_start[substart_byte+3] << 16) | (NIT_payload_start[substart_byte+4] << 8) | NIT_payload_start[substart_byte+5]) & SYMBOLRATE_MASK;
            printf("    %d)transmission_system (%d)/(%s) \n", MMS_Num+1, nit_mms_data->mms_data[MMS_Num].transmission_system, transmission_system[nit_mms_data->mms_data[MMS_Num].transmission_system]);
            printf("    %d)inner_coding_mode (%d)/(%s) \n", MMS_Num+1, nit_mms_data->mms_data[MMS_Num].inner_coding_mode, inner_coding_mode[nit_mms_data->mms_data[MMS_Num].inner_coding_mode]);
            printf("    %d)split_bitstream_mode (%d) \n", MMS_Num+1, nit_mms_data->mms_data[MMS_Num].split_bitstream_mode);
            printf("    %d)modulation_format (%d)/(%s) \n", MMS_Num+1, nit_mms_data->mms_data[MMS_Num].modulation_format, modulation_format[nit_mms_data->mms_data[MMS_Num].modulation_format]);
            printf("    %d)symbol_rate (%d) \n", MMS_Num+1, nit_mms_data->mms_data[MMS_Num].symbol_rate);
            substart_byte+= 6;
            descriptor_num-= 6;
            MMS_Num++;
        }
        else
        {
            perror("NIT-MMS invalid sub table format\n");
            return 1;
        }
        nit_mms_data->descriptor_count = NIT_payload_start[substart_byte];
        //printf("    %d)descriptor_count (%d) \n", MMS_Num, nit_mms_data->descriptor_count);
        substart_byte += 1;
        descriptor_num -= 1;
        for(j=0; j<nit_mms_data->descriptor_count; j++)
        {
            descriptor_tag = NIT_payload_start[substart_byte];
            descriptor_len = NIT_payload_start[substart_byte+1];
            switch(descriptor_tag)
            {
                case 0x80:
                        printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        printf("        %d)(detail info not yet implement)\n", j+1);
                    break;
                case 0x93:
                        printf("        %d)descriptor tag (Revision Detection Descriptor)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        printf("        %d)table_version_number (%d)\n", j+1, NIT_payload_start[substart_byte+2] & 0x1f);
                        printf("        %d)section_number(%d)\n", j+1, NIT_payload_start[substart_byte+3]);
                        printf("        %d)last_section_number(%d)\n", j+1, NIT_payload_start[substart_byte+4]);
                    break;
                default:
                    if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                    {
                        printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                        printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        printf("        %d)private data>\n                              ", j+1);                    
                        for(k=0; k< descriptor_len; k++)
                        {
                           printf("(0x%x)", NIT_payload_start[substart_byte+2+k]);
                        }
                        printf("\n");
                    }
                    break;
            }
            descriptor_num = descriptor_num - descriptor_len - 2;
            substart_byte = substart_byte + descriptor_len + 2;
        }
    }
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    nit_mms_data->outer_descriptor_num = descriptor_num;
    nit_mms_data->MMS_Num = MMS_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&NIT_payload_start[substart_byte], descriptor_num);
    printf("MMS numbers (%d) \n", nit_mms_data->MMS_Num);
    //nit_mms_data->CRC = (NIT_payload_start[substart_byte] << 24) | (NIT_payload_start[substart_byte+1] << 16 ) | (NIT_payload_start[substart_byte+2] << 8) | NIT_payload_start[substart_byte+3];
    nit_mms_data->CRC = (NIT_payload_start[crc_start] << 24) | (NIT_payload_start[crc_start+1] << 16 ) | (NIT_payload_start[crc_start+2] << 8) | NIT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", nit_mms_data->CRC);
    //printf("outside byte (%x) (%x) \n", NIT_payload_start[crc_start+4], NIT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

int NTT_SNS_Parser(SCTE_NTT_SNS *ntt_sns_data, uchar *NTT_payload, ushort NTT_pid)
{
    uchar *NTT_payload_start = NTT_payload;
    int i = 0;
    int j = 0;
    int k = 0;
    int SNS_Num = 0;
    int substart_byte = 8;
    int descriptor_num = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    int crc_start = 0;
    if(NTT_payload == NULL || ntt_sns_data == NULL)
    {
        perror("NTT-SNS input data error\n");
        return 1;
    }
    ntt_sns_data->NTT_PID = NTT_pid;
    ntt_sns_data->table_id = NTT_payload_start[0];
    ntt_sns_data->section_lenth = ( NTT_payload_start[1] << 8 | NTT_payload_start[2]) & SECTION_LEN_MASK;
    ntt_sns_data->protocol_version =   NTT_payload_start[3] & PROTOCOL_MASK;
    //ntt_sns_data->iso639_language_code = (NTT_payload_start[4] << 16) | (NTT_payload_start[5] << 8) | NTT_payload_start[6];
    sprintf(ntt_sns_data->iso639_language_code, "%c%c%c\0", NTT_payload_start[4], NTT_payload_start[5], NTT_payload_start[6]);
    ntt_sns_data->transmission_medium = NTT_payload_start[7] >> 4;
    ntt_sns_data->table_subtype = NTT_payload_start[7] & SUBTYPE_MASK;
    if(ntt_sns_data->protocol_version != 0x0)
    {
        perror("NTT-SNS PROTOCOL version error!!!\n");
        return 1;
    }
    
    if(ntt_sns_data->section_lenth < 9 || ntt_sns_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("NTT-SNS section length format error\n");
        return 1;
    }
    crc_start = 3 + ntt_sns_data->section_lenth - 4;
    descriptor_num = ntt_sns_data->section_lenth - 9;
    printf("=============NTT-SNS============\n");
    printf("NTT_PID (0x%x) \n",ntt_sns_data->NTT_PID);
    printf("table_id (0x%x) \n",ntt_sns_data->table_id);
    printf("section_lenth (%d) \n",ntt_sns_data->section_lenth);
    printf("protocol_version (%d) \n",ntt_sns_data->protocol_version);
    printf("iso639_language_code (%s) \n", ntt_sns_data->iso639_language_code);
    printf("transmission_medium (0x%x) \n",ntt_sns_data->transmission_medium);
    printf("table_subtype (%d) \n",ntt_sns_data->table_subtype);
   
    if(ntt_sns_data->table_subtype == 6)
    {
        ntt_sns_data->number_of_sns = NTT_payload_start[8];
        printf("number_of_sns (%d)\n", ntt_sns_data->number_of_sns);
        descriptor_num -= 1;
        substart_byte += 1;
    }
    else
    {
        perror("NTT-SNS invalid sub table format\n");
        return 1;
    }
    printf("SNS>\n");
    for(i=0; i<ntt_sns_data->number_of_sns; i++)
    {
            //SNS 6 byte
            ntt_sns_data->sns_data[SNS_Num].application_type = NTT_payload_start[substart_byte] >> 7;
            ntt_sns_data->sns_data[SNS_Num].application_source_id = (NTT_payload_start[substart_byte+1] << 8 ) | NTT_payload_start[substart_byte+2];
            ntt_sns_data->sns_data[SNS_Num].name_length = NTT_payload_start[substart_byte+3];
            memcpy(ntt_sns_data->sns_data[SNS_Num].source_name, &NTT_payload_start[substart_byte+4], ntt_sns_data->sns_data[SNS_Num].name_length);

            substart_byte = substart_byte + ntt_sns_data->sns_data[SNS_Num].name_length + 4;
            descriptor_num = descriptor_num - ntt_sns_data->sns_data[SNS_Num].name_length - 4;
            ntt_sns_data->sns_data[SNS_Num].SNS_descriptor_count = NTT_payload_start[substart_byte] ;
            substart_byte += 1;
            descriptor_num -= 1;
            
            printf("    %d)application_type (%d) \n", SNS_Num+1, ntt_sns_data->sns_data[SNS_Num].application_type);
            if(ntt_sns_data->sns_data[SNS_Num].application_type)
                printf("    %d)application id (0x%x) \n", SNS_Num+1, ntt_sns_data->sns_data[SNS_Num].application_source_id);
            else
                printf("    %d)source id (0x%x) \n", SNS_Num+1, ntt_sns_data->sns_data[SNS_Num].application_source_id);
            printf("    %d)name_length (%d) \n", SNS_Num+1, ntt_sns_data->sns_data[SNS_Num].name_length);
            
            printf("    %d)source_name (", SNS_Num+1, &ntt_sns_data->sns_data[SNS_Num].source_name[2]);
            for (j=0; j<ntt_sns_data->sns_data[SNS_Num].name_length; j++)
	     {
			if (j > 1)
				printf("%c", ntt_sns_data->sns_data[SNS_Num].source_name[j]);
	      }
	      printf(")\n");
            //printf("    %d)SNS_descriptor_count (%d) \n", SNS_Num+1, ntt_sns_data->sns_data[SNS_Num].SNS_descriptor_count);            
            for(j=0; j<ntt_sns_data->sns_data[SNS_Num].SNS_descriptor_count; j++)
            {
                descriptor_tag = NTT_payload_start[substart_byte];
                descriptor_len = NTT_payload_start[substart_byte+1];
                switch(descriptor_tag)
                {
                    case 0x80:
                            printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                            printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                        break;
                    case 0x93:
                            printf("        %d)descriptor tag (Revision Detection Descriptor)\n", j+1);
                            printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                            printf("        %d)table_version_number (%d)\n", j+1, NTT_payload_start[substart_byte+2] & 0x1f);
                            printf("        %d)section_number(%d)\n", j+1, NTT_payload_start[substart_byte+3]);
                            printf("        %d)last_section_number(%d)\n", j+1, NTT_payload_start[substart_byte+4]);
                        break;
                    default:
                        if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                        {
                            printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                            printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                            printf("        %d)private data>\n                              ", j+1);                    
                            for(k=0; k< descriptor_len; k++)
                            {
                               printf("(0x%x)", NTT_payload_start[substart_byte+2+k]);
                            }
                            printf("\n");
                        }
                        break;
                }
                descriptor_num = (descriptor_num - descriptor_len - 2);
                substart_byte = (substart_byte + descriptor_len + 2);
            }
            SNS_Num++;
    }
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    ntt_sns_data->outer_descriptor_num = descriptor_num;
    ntt_sns_data->SNS_Num = SNS_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&NTT_payload_start[substart_byte], descriptor_num);
    printf("SNS numbers (%d) \n", ntt_sns_data->SNS_Num);
    ntt_sns_data->CRC = (NTT_payload_start[substart_byte] << 24) | (NTT_payload_start[substart_byte+1] << 16 ) | (NTT_payload_start[substart_byte+2] << 8) | NTT_payload_start[substart_byte+3];
    printf("CRC32 (0x%x) \n", ntt_sns_data->CRC);
    //printf("outside byte (%x) (%x) \n", NTT_payload_start[substart_byte+4], NTT_payload_start[substart_byte+5]);
    printf("================================\n");
    return 0;
}

int SVCT_DCM_Parser(SCTE_SVCT_DCM *svct_dcm_data, uchar *SVCT_payload, ushort SVCT_pid)
{
    uchar *SVCT_payload_start = SVCT_payload;
    int i = 0;
    int j = 0;
    int DCM_Num = 0;
    int substart_byte = 7;
    int descriptor_num = 0;
    int crc_start = 0;
    if(SVCT_payload == NULL || svct_dcm_data == NULL)
    {
        perror("SVCT-DCM input data error\n");
        return 1;
    }
    svct_dcm_data->SVCT_PID = SVCT_pid;
    svct_dcm_data->table_id = SVCT_payload_start[0];
    svct_dcm_data->section_lenth = ( SVCT_payload_start[1] << 8 | SVCT_payload_start[2]) & SECTION_LEN_MASK;
    svct_dcm_data->protocol_version =   SVCT_payload_start[3] & PROTOCOL_MASK;
    svct_dcm_data->transmission_medium = SVCT_payload_start[4] >> 4;
    svct_dcm_data->table_subtype = SVCT_payload_start[4] & SUBTYPE_MASK;
    svct_dcm_data->VCT_ID = (SVCT_payload_start[5] << 8) | SVCT_payload_start[6];
    if(svct_dcm_data->protocol_version != 0x0)
    {
        perror("SVCT-DCM PROTOCOL version error!!!\n");
        return 1;
    }
    if(svct_dcm_data->section_lenth < 11 || svct_dcm_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("SVCT-DCM section length format error\n");
        return 1;
    }
    crc_start = 3 + svct_dcm_data->section_lenth - 4;
    descriptor_num = svct_dcm_data->section_lenth - 8;
    printf("============SVCT-DCM============\n");
    printf("SVCT_PID (0x%x) \n",svct_dcm_data->SVCT_PID);
    printf("table_id (0x%x) \n",svct_dcm_data->table_id);
    printf("section_lenth (%d) \n",svct_dcm_data->section_lenth);
    printf("protocol_version (%d) \n",svct_dcm_data->protocol_version);
    printf("transmission_medium (0x%x) \n",svct_dcm_data->transmission_medium);
    printf("table_subtype (%d) \n",svct_dcm_data->table_subtype);
    printf("VCT_ID (0x%x) \n",svct_dcm_data->VCT_ID);
    printf("DCM>\n");

    if(svct_dcm_data->table_subtype == 1)
    { 
        //DCM 5 byte
        svct_dcm_data->first_virtual_channel = ((SVCT_payload_start[substart_byte] << 8)  | SVCT_payload_start[substart_byte+1]) & FIRST_VCH_MASK;
        svct_dcm_data->DCM_data_length = SVCT_payload_start[substart_byte+2] & DCM_LENGTH_MASK;
        printf("first_virtual_channel (%d) \n",svct_dcm_data->first_virtual_channel);
        printf("DCM_data_length (%d) \n",svct_dcm_data->DCM_data_length);    
        substart_byte += 3;
        descriptor_num -= 3;
        for(i=0; i<svct_dcm_data->DCM_data_length; i++)
        {
            svct_dcm_data->dcm_data[DCM_Num].range_defined = SVCT_payload_start[substart_byte] >> 7; //change to bool ?
            svct_dcm_data->dcm_data[DCM_Num].channels_count = SVCT_payload_start[substart_byte] & DCM_LENGTH_MASK;
            printf("    %d)range_defined (%d) \n", DCM_Num+1, svct_dcm_data->dcm_data[DCM_Num].range_defined);
            printf("    %d)channels_count (%d) \n", DCM_Num+1, svct_dcm_data->dcm_data[DCM_Num].channels_count);
            DCM_Num++;
            substart_byte += 1;
            descriptor_num -= 1;
        }
    }
    else
    {
        perror("SVCT-DCM invalid sub table format\n");
        return 1;
    }
        
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    svct_dcm_data->outer_descriptor_num = descriptor_num;
    svct_dcm_data->DCM_Num = DCM_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&SVCT_payload_start[substart_byte], descriptor_num);
    //svct_dcm_data->CRC = (SVCT_payload_start[substart_byte] << 24) | (SVCT_payload_start[substart_byte+1] << 16 ) | (SVCT_payload_start[substart_byte+2] << 8) | SVCT_payload_start[substart_byte+3];
    svct_dcm_data->CRC = (SVCT_payload_start[crc_start] << 24) | (SVCT_payload_start[crc_start+1] << 16 ) | (SVCT_payload_start[crc_start+2] << 8) | SVCT_payload_start[crc_start+3];
    printf("DCM numbers (%d) \n", svct_dcm_data->DCM_Num);
    printf("CRC32 (0x%x) \n", svct_dcm_data->CRC);
    //printf("outside byte (%x) (%x) \n", SVCT_payload_start[crc_start+4], SVCT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

int SVCT_VCM_Parser(SCTE_SVCT_VCM *svct_vcm_data, uchar *SVCT_payload, ushort SVCT_pid)
{
    uchar *SVCT_payload_start = SVCT_payload;
    int i = 0;
    int j = 0;
    int k = 0;
    int VCM_Num = 0;
    int substart_byte = 7;
    int descriptor_num = 0;
    uchar descriptor_tag;
    uchar descriptor_len;
    int crc_start = 0;
    if(SVCT_payload == NULL || svct_vcm_data == NULL)
    {
        perror("SVCT-VCM input data error\n");
        return 1;
    }
    svct_vcm_data->SVCT_PID = SVCT_pid;
    svct_vcm_data->table_id = SVCT_payload_start[0];
    svct_vcm_data->section_lenth = ( SVCT_payload_start[1] << 8 | SVCT_payload_start[2]) & SECTION_LEN_MASK;
    svct_vcm_data->protocol_version =   SVCT_payload_start[3] & PROTOCOL_MASK;
    svct_vcm_data->transmission_medium = SVCT_payload_start[4] >> 4;
    svct_vcm_data->table_subtype = SVCT_payload_start[4] & SUBTYPE_MASK;
    svct_vcm_data->VCT_ID = (SVCT_payload_start[5] << 8) | SVCT_payload_start[6];
    if(svct_vcm_data->protocol_version != 0x0)
    {
        perror("SVCT-VCM PROTOCOL version error!!!\n");
        return 1;
    }
    if(svct_vcm_data->section_lenth < 15 || svct_vcm_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("SVCT-VCM section length format error\n");
        return 1;
    }
    crc_start = 3 + svct_vcm_data->section_lenth - 4;
    descriptor_num = svct_vcm_data->section_lenth - 8;
    printf("============SVCT-VCM============\n");
    printf("SVCT_PID (0x%x) \n",svct_vcm_data->SVCT_PID);
    printf("table_id (0x%x) \n",svct_vcm_data->table_id);
    printf("section_lenth (%d) \n",svct_vcm_data->section_lenth);
    printf("protocol_version (%d) \n",svct_vcm_data->protocol_version);
    printf("transmission_medium (0x%x) \n",svct_vcm_data->transmission_medium);
    printf("table_subtype (%d) \n",svct_vcm_data->table_subtype);
    printf("VCT_ID (0x%x) \n",svct_vcm_data->VCT_ID);
    printf("VCM>\n");

    if(svct_vcm_data->table_subtype == 0)
    { 
        //VCM 7 byte
        svct_vcm_data->descriptors_included = (SVCT_payload_start[substart_byte] >> 5) & DESCRIPTORS_INCLUDED_MASK;
        svct_vcm_data->splice = SVCT_payload_start[substart_byte+1] >> 7;
        svct_vcm_data->activation_time = (SVCT_payload_start[substart_byte+2] << 24) | (SVCT_payload_start[substart_byte+3] << 16 ) | (SVCT_payload_start[substart_byte+4] << 8) | SVCT_payload_start[substart_byte+5];
        svct_vcm_data->number_of_VC_records = SVCT_payload_start[substart_byte+6];
        printf("descriptors_included (%d) \n",svct_vcm_data->descriptors_included);
        printf("splice (%d) \n",svct_vcm_data->splice);    
        printf("activation_time (%d) \n",svct_vcm_data->activation_time);
        printf("number_of_VC_records (%d) \n",svct_vcm_data->number_of_VC_records);
        substart_byte += 7;
        descriptor_num -= 7;
        for(i=0; i<svct_vcm_data->number_of_VC_records; i++)
        {
            svct_vcm_data->Virtual_CH[VCM_Num].virtual_channel_number = ((SVCT_payload_start[substart_byte]<<8) | SVCT_payload_start[substart_byte+1] ) & VIRTUAL_CHANNEL_MASK; //change to bool ?
            svct_vcm_data->Virtual_CH[VCM_Num].application_virtual_channel = SVCT_payload_start[substart_byte+2] >> 7; //change to bool ?
            svct_vcm_data->Virtual_CH[VCM_Num].path_select = (SVCT_payload_start[substart_byte+2] >> 5) & PATH_SELECT_MASK; //change to bool ?
            svct_vcm_data->Virtual_CH[VCM_Num].transport_type = (SVCT_payload_start[substart_byte+2] >> 4) & TRANSPORT_TYPE_MASK; //change to bool ?
            svct_vcm_data->Virtual_CH[VCM_Num].channel_type = SVCT_payload_start[substart_byte+2] & CHANNEL_TYPE_MASK;
            svct_vcm_data->Virtual_CH[VCM_Num].application_source_id = SVCT_payload_start[substart_byte+3] << 8 | SVCT_payload_start[substart_byte+4];
            printf("    %d)virtual_channel_number (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].virtual_channel_number);
            printf("    %d)application_virtual_channel (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].application_virtual_channel);
            printf("    %d)path_select (%d)/(%s) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].path_select, path_select[svct_vcm_data->Virtual_CH[VCM_Num].path_select]);
            printf("    %d)transport_type (%d)/(%s) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].transport_type, transport_type[svct_vcm_data->Virtual_CH[VCM_Num].transport_type]);
            printf("    %d)channel_type (%d)/(%s) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].channel_type, channel_type[svct_vcm_data->Virtual_CH[VCM_Num].channel_type]);
            if(svct_vcm_data->Virtual_CH[VCM_Num].application_virtual_channel)
                printf("    %d)application_ID (0x%x) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].application_source_id);
            else
                printf("    %d)source_ID (0x%x) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].application_source_id);
            if(!svct_vcm_data->Virtual_CH[VCM_Num].transport_type)
            {
                svct_vcm_data->Virtual_CH[VCM_Num].CDS_reference = SVCT_payload_start[substart_byte+5];
                svct_vcm_data->Virtual_CH[VCM_Num].program_number = (SVCT_payload_start[substart_byte+6] << 8) | SVCT_payload_start[substart_byte+7];
                svct_vcm_data->Virtual_CH[VCM_Num].MMS_reference = SVCT_payload_start[substart_byte+8];
                printf("    %d)CDS_reference (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].CDS_reference);
                printf("    %d)program_number (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].program_number);
                printf("    %d)MMS_reference (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].MMS_reference);
            }
            else
            {
                svct_vcm_data->Virtual_CH[VCM_Num].CDS_reference = SVCT_payload_start[substart_byte+5];
                svct_vcm_data->Virtual_CH[VCM_Num].scrambled = SVCT_payload_start[substart_byte+6] >> 7;
                svct_vcm_data->Virtual_CH[VCM_Num].video_standard = SVCT_payload_start[substart_byte+6] & VIDEO_STANDARD_MASK;
                printf("    %d)CDS_reference (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].CDS_reference);
                printf("    %d)scrambled (%d) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].scrambled);
                printf("    %d)video_standard (%d)/(%s) \n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].video_standard, video_standard[svct_vcm_data->Virtual_CH[VCM_Num].video_standard]);
                
            }
            substart_byte += 9;
            descriptor_num -= 9;
            if(svct_vcm_data->descriptors_included)
            {
                svct_vcm_data->Virtual_CH[VCM_Num].descriptors_count = SVCT_payload_start[substart_byte];
                substart_byte += 1;
                descriptor_num -= 1;
                printf("    %d)descriptor counter (%d)\n", VCM_Num+1, svct_vcm_data->Virtual_CH[VCM_Num].descriptors_count);
                for(j=0; j<svct_vcm_data->Virtual_CH[VCM_Num].descriptors_count; j++)
                {
                    descriptor_tag = SVCT_payload_start[substart_byte];
                    descriptor_len = SVCT_payload_start[substart_byte+1];
                    switch(descriptor_tag)
                    {
                        case 0x80:
                                printf("        %d)descriptor tag (stuffing descriptor)\n", j+1);
                                printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                                printf("        %d)(detail info not yet implement)\n", j+1);
                                break;
                        case 0x93:
                                printf("        %d)descriptor tag (Revision Detection Descriptor)\n", j+1);
                                printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                                printf("        %d)table_version_number (%d)\n", j+1, SVCT_payload_start[substart_byte+2] & 0x1f);
                                printf("        %d)section_number(%d)\n", j+1, SVCT_payload_start[substart_byte+3]);
                                printf("        %d)last_section_number(%d)\n", j+1, SVCT_payload_start[substart_byte+4]);
                                break;
                        case 0x94:
                                printf("        %d)descriptor tag (Two-part Channel Number Descriptor)\n", j+1);
                                printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                                printf("        %d)major_channel_number (%d)\n", j+1, (SVCT_payload_start[substart_byte+2] << 8 | SVCT_payload_start[substart_byte+3]) & 0x3ff);
                                printf("        %d)minor_channel_number(%d)\n", j+1, (SVCT_payload_start[substart_byte+4] << 8 | SVCT_payload_start[substart_byte+5]) & 0x3ff);
                                break;
                        case 0x95:
                                printf("        %d)descriptor tag (channel_properties_descriptor)\n", j+1);
                                printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                                printf("        %d)channel_TSID (%d)\n", j+1, (SVCT_payload_start[substart_byte+2] << 8 | SVCT_payload_start[substart_byte+3]));
                                printf("        %d)out_of_band_channel (%d)\n", j+1, (SVCT_payload_start[substart_byte+4] >> 1) & 0x1);
                                printf("        %d)access_controlled (%d)\n", j+1, SVCT_payload_start[substart_byte+4] & 0x1);
                                printf("        %d)hide_guide (%d)\n", j+1, (SVCT_payload_start[substart_byte+5] >> 7) & 0x1);
                                printf("        %d)service_type (%d)\n", j+1, SVCT_payload_start[substart_byte+5] & 0x3f);
                                break;
                        default:
                            if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                            {
                                printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                                printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                                for(k=0; k< descriptor_len; k++)
                                {
                                   printf("(0x%x)", SVCT_payload_start[substart_byte+2+k]);
                                }
                                printf("\n");
                            }
                            break;
                    }
                    descriptor_num = descriptor_num - descriptor_len - 2;  
                    substart_byte = substart_byte + descriptor_len + 2;
                }
            }
            VCM_Num++;
        }
    }
    else
    {
        perror("SVCT-VCM invalid sub table format\n");
        return 1;
    }   
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    svct_vcm_data->outer_descriptor_num = descriptor_num;
    svct_vcm_data->VCM_Num = VCM_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&SVCT_payload_start[substart_byte], descriptor_num);
    printf("VCM numbers (%d) \n", svct_vcm_data->VCM_Num);
    //svct_vcm_data->CRC = (SVCT_payload_start[substart_byte] << 24) | (SVCT_payload_start[substart_byte+1] << 16 ) | (SVCT_payload_start[substart_byte+2] << 8) | SVCT_payload_start[substart_byte+3];
    svct_vcm_data->CRC = (SVCT_payload_start[crc_start] << 24) | (SVCT_payload_start[crc_start+1] << 16 ) | (SVCT_payload_start[crc_start+2] << 8) | SVCT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", svct_vcm_data->CRC);
    //printf("outside byte (%x) (%x) \n", SVCT_payload_start[crc_start+4], SVCT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

int SVCT_ICM_Parser(SCTE_SVCT_ICM *svct_icm_data, uchar *SVCT_payload, ushort SVCT_pid)
{
    uchar *SVCT_payload_start = SVCT_payload;
    int i = 0;
    int j = 0;
    int ICM_Num = 0;
    int substart_byte = 7;
    int descriptor_num = 0;
    int crc_start = 0;
    if(SVCT_payload == NULL || svct_icm_data == NULL)
    {
        perror("SVCT-ICM input data error\n");
        return 1;
    }
    svct_icm_data->SVCT_PID = SVCT_pid;
    svct_icm_data->table_id = SVCT_payload_start[0];
    svct_icm_data->section_lenth = ( SVCT_payload_start[1] << 8 | SVCT_payload_start[2]) & SECTION_LEN_MASK;
    svct_icm_data->protocol_version =   SVCT_payload_start[3] & PROTOCOL_MASK;
    svct_icm_data->transmission_medium = SVCT_payload_start[4] >> 4;
    svct_icm_data->table_subtype = SVCT_payload_start[4] & SUBTYPE_MASK;
    svct_icm_data->VCT_ID = (SVCT_payload_start[5] << 8) | SVCT_payload_start[6];
    if(svct_icm_data->protocol_version != 0x0)
    {
        perror("SVCT-ICM PROTOCOL version error!!!\n");
        return 1;
    }
    if(svct_icm_data->section_lenth < 11 || svct_icm_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("SVCT-ICM section length format error\n");
        return 1;
    }
    crc_start = 3 + svct_icm_data->section_lenth - 4;
    descriptor_num = svct_icm_data->section_lenth - 8;
    printf("============SVCT-ICM============\n");
    printf("SVCT_PID (0x%x) \n",svct_icm_data->SVCT_PID);
    printf("table_id (0x%x) \n",svct_icm_data->table_id);
    printf("section_lenth (%d) \n",svct_icm_data->section_lenth);
    printf("protocol_version (%d) \n",svct_icm_data->protocol_version);
    printf("transmission_medium (0x%x) \n",svct_icm_data->transmission_medium);
    printf("table_subtype (%d) \n",svct_icm_data->table_subtype);
    printf("VCT_ID (0x%x) \n",svct_icm_data->VCT_ID);
    printf("ICM>\n");

    if(svct_icm_data->table_subtype == 2)
    { 
        //ICM 3 byte
        svct_icm_data->first_map_index = (SVCT_payload_start[substart_byte] << 8)  | SVCT_payload_start[substart_byte+1];
        svct_icm_data->record_count = SVCT_payload_start[substart_byte+2];
        printf("first_map_index (%d) \n",svct_icm_data->first_map_index);
        printf("record_count (%d) \n",svct_icm_data->record_count);    
        substart_byte += 3;
        descriptor_num -= 3;
        for(i=0; i<svct_icm_data->record_count; i++)
        {
            svct_icm_data->icm_data[ICM_Num].source_id = SVCT_payload_start[substart_byte] << 8 | SVCT_payload_start[substart_byte+1];  
            svct_icm_data->icm_data[ICM_Num].virtual_channel_number = SVCT_payload_start[substart_byte+2] << 8 | SVCT_payload_start[substart_byte+3];
            printf("    %d)source_id (0x%x) \n", ICM_Num+1, svct_icm_data->icm_data[ICM_Num].source_id);
            printf("    %d)virtual_channel_number (0x%x) \n", ICM_Num+1, svct_icm_data->icm_data[ICM_Num].virtual_channel_number);
            ICM_Num++;
            substart_byte += 4;
            descriptor_num -= 4;
        }
    }
    else
    {
        perror("SVCT-ICM invalid sub table format\n");
        return 1;
    }
        
    //substart_byte += descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    svct_icm_data->outer_descriptor_num = descriptor_num;
    svct_icm_data->ICM_Num = ICM_Num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&SVCT_payload_start[substart_byte], descriptor_num);
    //svct_icm_data->CRC = (SVCT_payload_start[substart_byte] << 24) | (SVCT_payload_start[substart_byte+1] << 16 ) | (SVCT_payload_start[substart_byte+2] << 8) | SVCT_payload_start[substart_byte+3];
    svct_icm_data->CRC = (SVCT_payload_start[crc_start] << 24) | (SVCT_payload_start[crc_start+1] << 16 ) | (SVCT_payload_start[crc_start+2] << 8) | SVCT_payload_start[crc_start+3];
    printf("ICM numbers (%d) \n", svct_icm_data->ICM_Num);
    printf("CRC32 (0x%x) \n", svct_icm_data->CRC);
    //printf("outside byte (%x) (%x) \n", SVCT_payload_start[crc_start+4], SVCT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

int SCTE_STT_Parser(SCTE_STT *scte_stt_data, uchar *STT_payload, ushort STT_pid)
{
    uchar *STT_payload_start = STT_payload;
    int i = 0;
    int j = 0;
    int substart_byte = 10;
    int descriptor_num = 0;
    int crc_start = 0;
    if(STT_payload == NULL || scte_stt_data == NULL)
    {
        perror("STT input data error\n");
        return 1;
    }
    scte_stt_data->STT_PID = STT_pid;
    scte_stt_data->table_id = STT_payload_start[0];
    scte_stt_data->section_lenth = ( STT_payload_start[1] << 8 | STT_payload_start[2]) & SECTION_LEN_MASK;
    scte_stt_data->protocol_version =   STT_payload_start[3] & PROTOCOL_MASK;
    scte_stt_data->system_time = (STT_payload_start[5] << 24) | (STT_payload_start[6] << 16) | (STT_payload_start[7] << 8) | STT_payload_start[8];
    scte_stt_data->GPS_UTC_offset = STT_payload_start[9];
    if(scte_stt_data->protocol_version != 0x0)
    {
        perror("STT PROTOCOL version error!!!\n");
        return 1;
    }
    if(scte_stt_data->section_lenth < 11 || scte_stt_data->section_lenth > NORMAL_SECTION_LENGTH)
    {
        perror("STT section length format error\n");
        return 1;
    }
    printf("==============STT===============\n");
    printf("STT_PID (0x%x) \n",scte_stt_data->STT_PID);
    printf("table_id (0x%x) \n",scte_stt_data->table_id);
    printf("section_lenth (%d) \n",scte_stt_data->section_lenth);
    printf("protocol_version (%d) \n",scte_stt_data->protocol_version);
    printf("system_time (%d) \n",scte_stt_data->system_time);
    printf("GPS_UTC_offset (%d) \n",scte_stt_data->GPS_UTC_offset);
    
    crc_start = 3 + scte_stt_data->section_lenth - 4;
    descriptor_num = scte_stt_data->section_lenth - 11;
    scte_stt_data->outer_descriptor_num = descriptor_num;
    printf("outer descriptor_num (%d) \n", descriptor_num);
    //substart_byte += descriptor_num;
    //reserve for descriptor
    substart_byte += SCTE65_descriptor_Parser(&STT_payload_start[substart_byte], descriptor_num);
    //scte_stt_data->CRC = (STT_payload_start[substart_byte] << 24) | (STT_payload_start[substart_byte+1] << 16 ) | (STT_payload_start[substart_byte+2] << 8) | STT_payload_start[substart_byte+3];
    scte_stt_data->CRC = (STT_payload_start[crc_start] << 24) | (STT_payload_start[crc_start+1] << 16 ) | (STT_payload_start[crc_start+2] << 8) | STT_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", scte_stt_data->CRC);
    //printf("outside byte (%x) (%x) \n", STT_payload_start[crc_start+4], STT_payload_start[crc_start+5]);
    printf("================================\n");
    return 0;
}

/*********************************
*             SCTE18 main function               *
*********************************/

int SCTE_EAS_Parser(SCTE_EAS *scte_eas_data, uchar *EAS_payload, ushort EAS_pid)
{
    uchar *EAS_payload_start = EAS_payload;
    int i = 0;
    int j = 0;
    int substart_byte = 15;
    int descriptor_num = 0;
    int County_Code_num = 0;
    int Exception_num = 0;
    int crc_start = 0;
    uchar number_of_segments[256];
    uchar compress_type[256];
    uchar mode[256];
    uchar number_of_bytes[256];
    uchar text_byte[256];
    uchar number_of_string;
    uchar descriptor_tag;
    uchar descriptor_len;
    uchar exception_descriptor_count;
    int k=1;
    int xy=0;
    if(EAS_payload == NULL || scte_eas_data == NULL)
    {
        perror("EAS input data error\n");
        return 1;
    }
    scte_eas_data->EAS_PID = EAS_pid;
    scte_eas_data->table_id = EAS_payload_start[0];
    scte_eas_data->section_lenth = ( EAS_payload_start[1] << 8 | EAS_payload_start[2]) & SECTION_LEN_MASK;  
    if(scte_eas_data->section_lenth < 40)
    {
        perror("EAS section length format error\n");
        return 1;
    }
    scte_eas_data->table_id_extention = (EAS_payload_start[3] << 8) | EAS_payload_start[4];
    scte_eas_data->sequence_number = ( (EAS_payload_start[5] >> 1) & SEQUENCE_NUMBER_MASK);
    scte_eas_data->next_indicator = EAS_payload_start[5]  & CURRENT_NEXT_MASK;
    if(scte_eas_data->next_indicator != 1)
    {
        perror("Not valid EAS\n");
        return 1;
    }
    scte_eas_data->section_number = EAS_payload_start[6];
    scte_eas_data->last_section_number = EAS_payload_start[7];
    if(scte_eas_data->last_section_number > 0)
    {
        printf("EAS section number is over 1, we don't support it \n");
        return 1;
    }
     crc_start = 3 + scte_eas_data->section_lenth - 4;
    scte_eas_data->protocol_version  = EAS_payload_start[8];
    if(scte_eas_data->protocol_version != 0x0)
    {
        perror("EAS PROTOCOL version error!!!\n");
        return 1;
    }
    scte_eas_data->EAS_event_ID = EAS_payload_start[9] << 8 | EAS_payload_start[10];
    //scte_eas_data->EAS_originator_code = sprintf(EAS_payload_start[11] << 16 | EAS_payload_start[12] << 8 | EAS_payload_start[13];
    sprintf(scte_eas_data->EAS_originator_code,"%c%c%c",EAS_payload_start[11], EAS_payload_start[12], EAS_payload_start[13]);
    scte_eas_data->EAS_event_code_length = EAS_payload_start[14];
    memcpy(scte_eas_data->EAS_event_code, &EAS_payload_start[15], scte_eas_data->EAS_event_code_length); //copy EAS event code
    substart_byte += scte_eas_data->EAS_event_code_length;
    scte_eas_data->nature_of_activation_text_length = EAS_payload_start[substart_byte];
    memcpy(scte_eas_data->nature_of_activation_text, &EAS_payload_start[substart_byte+1], scte_eas_data->nature_of_activation_text_length);
    substart_byte = substart_byte + scte_eas_data->nature_of_activation_text_length + 1;
    scte_eas_data->alert_message_time_remaining = EAS_payload_start[substart_byte];
    scte_eas_data->event_start_time = EAS_payload_start[substart_byte+1] << 24 | EAS_payload_start[substart_byte+2] << 16 | EAS_payload_start[substart_byte+3] << 8 | EAS_payload_start[substart_byte+4];
    scte_eas_data->event_duration = EAS_payload_start[substart_byte+5] << 8 | EAS_payload_start[substart_byte+6];
    scte_eas_data->alert_priority = EAS_payload_start[substart_byte+8] & ALERT_PRIORITY_MASK;
    scte_eas_data->details_OOB_source_ID = EAS_payload_start[substart_byte+9] << 8 | EAS_payload_start[substart_byte+10];
    scte_eas_data->details_major_channel_number = (EAS_payload_start[substart_byte+11] << 8 | EAS_payload_start[substart_byte+12]) & DETAIL_CHANNEL_MASK;
    scte_eas_data->details_minor_channel_number = (EAS_payload_start[substart_byte+13] << 8 | EAS_payload_start[substart_byte+14]) & DETAIL_CHANNEL_MASK;
    scte_eas_data->audio_OOB_source_ID = EAS_payload_start[substart_byte+15] << 8 | EAS_payload_start[substart_byte+16];
    scte_eas_data->alert_text_length = EAS_payload_start[substart_byte+17] << 8 | EAS_payload_start[substart_byte+18];
    memcpy(scte_eas_data->alert_text, &EAS_payload_start[substart_byte+19], scte_eas_data->alert_text_length);
    
    substart_byte = substart_byte + 19 + scte_eas_data->alert_text_length ;
    scte_eas_data->location_code_count = EAS_payload_start[substart_byte];
    substart_byte += 1;
    printf("==============EAS===============\n");
    printf("EAS_PID (0x%x) \n",scte_eas_data->EAS_PID);
    printf("table_id (0x%x) \n",scte_eas_data->table_id);
    printf("section_lenth (%d) \n",scte_eas_data->section_lenth);
    printf("table_id_extention (0x%x) \n",scte_eas_data->table_id_extention);
    printf("sequence_number (%d) \n",scte_eas_data->sequence_number);
    printf("current_next_indicator (%d) \n",scte_eas_data->next_indicator);
    printf("section_number (%d) \n",scte_eas_data->section_number);
    printf("last_section_number (%d) \n",scte_eas_data->last_section_number);
    printf("protocol_version (%d) \n",scte_eas_data->protocol_version);
    printf("EAS_event_ID (0x%x) \n",scte_eas_data->EAS_event_ID);
    printf("EAS_originator_code (%s) \n",scte_eas_data->EAS_originator_code);
    printf("EAS_event_code_length (%d) \n",scte_eas_data->EAS_event_code_length);
    printf("EAS_event_code (%s) \n",scte_eas_data->EAS_event_code);
    printf("nature_of_activation_text_length (%d) \n",scte_eas_data->nature_of_activation_text_length);
    //printf("nature_of_activation_text (%s) \n",scte_eas_data->nature_of_activation_text);
    number_of_string = scte_eas_data->nature_of_activation_text[0];
    k=1;
    printf("nature_of_activation_text>\n");
    //printf("    number_of_string (%d) \n", number_of_string);
    for( i=0; i<number_of_string; i++)
    {   
         printf("    %d)iso639_code (", i+1);
         printf("%c",scte_eas_data->nature_of_activation_text[k]);
         printf("%c",scte_eas_data->nature_of_activation_text[k+1]);
         printf("%c",scte_eas_data->nature_of_activation_text[k+2]);
         printf(")\n");
         number_of_segments[i] = scte_eas_data->nature_of_activation_text[k+3];
         //printf("    %d)number_of_segments (%d)\n", i+1, number_of_segments[i]);
         k+=4;
         for(j=0; j< number_of_segments[i]; j++)
         {
              compress_type[j] = scte_eas_data->nature_of_activation_text[k];
              mode[j] = scte_eas_data->nature_of_activation_text[k+1];
              number_of_bytes[j] = scte_eas_data->nature_of_activation_text[k+2];
              printf("        %d)compression_type (%d)\n", j+1, compress_type[j]);
              printf("        %d)mode (%d)\n", j+1, mode[j]);
              printf("        %d)number_bytes (%d)\n", j+1, number_of_bytes[j]);
              memcpy(text_byte, &scte_eas_data->nature_of_activation_text[k+3], number_of_bytes[j]);
              printf("        %d)compressed_string (", j+1);
              for(xy=0; xy<number_of_bytes[j];xy++)
              {
                printf("%c", text_byte[xy]);
              }
              printf(")\n");
              k = k + 4 + number_of_bytes[j];
         }
    }
    printf("alert_message_time_remaining (%d) \n",scte_eas_data->alert_message_time_remaining);
    printf("event_start_time (%d) \n",scte_eas_data->event_start_time);
    printf("event_duration (%d) \n",scte_eas_data->event_duration);
    printf("alert_priority (%d) \n",scte_eas_data->alert_priority);
    printf("details_OOB_source_ID (0x%x) \n",scte_eas_data->details_OOB_source_ID);
    printf("details_major_channel_number (%d) \n",scte_eas_data->details_major_channel_number);
    printf("details_minor_channel_number (%d) \n",scte_eas_data->details_minor_channel_number);
    printf("audio_OOB_source_ID (0x%x) \n",scte_eas_data->audio_OOB_source_ID);
    printf("alert_text_length (%d) \n",scte_eas_data->alert_text_length);
    number_of_string = scte_eas_data->alert_text[0];
    k=1;
    printf("alert_text>\n");
    //printf("    number_of_string (%d) \n", number_of_string);
    for( i=0; i<number_of_string; i++)
    {   
         printf("    %d)iso639_code (", i+1);
         printf("%c",scte_eas_data->alert_text[k]);
         printf("%c",scte_eas_data->alert_text[k+1]);
         printf("%c",scte_eas_data->alert_text[k+2]);
         printf(")\n");
         number_of_segments[i] = scte_eas_data->alert_text[k+3];
         //printf("    %d)number_of_segments (%d)\n", i+1, number_of_segments[i]);
         k+=4;
         for(j=0; j< number_of_segments[i]; j++)
         {
              compress_type[j] = scte_eas_data->alert_text[k];
              mode[j] = scte_eas_data->alert_text[k+1];
              number_of_bytes[j] = scte_eas_data->alert_text[k+2];
              printf("        %d)compression_type (%d)\n", j+1, compress_type[j]);
              printf("        %d)mode (%d)\n", j+1, mode[j]);
              printf("        %d)number_bytes (%d)\n", j+1, number_of_bytes[j]);
              memcpy(text_byte, &scte_eas_data->alert_text[k+3], number_of_bytes[j]);
              printf("        %d)compressed_string (%s)\n", j+1, text_byte);
              k = k + 4 + number_of_bytes[j];
         }
    }
    //printf("location_code_count (%d) \n",scte_eas_data->location_code_count);
    printf("Count_Code>\n");
    for(i=0; i< scte_eas_data->location_code_count; i++)
    {
        scte_eas_data->location_code_data[County_Code_num].state_code = EAS_payload_start[substart_byte];
        scte_eas_data->location_code_data[County_Code_num].county_subdivision = EAS_payload_start[substart_byte+1] >> 4;
        scte_eas_data->location_code_data[County_Code_num].county_code = EAS_payload_start[substart_byte+2] & COUNTY_CODE_MASK;
        printf("    %d)state_code (%d) \n", County_Code_num+1, scte_eas_data->location_code_data[County_Code_num].state_code);
        printf("    %d)county_subdivision (%d) \n", County_Code_num+1, scte_eas_data->location_code_data[County_Code_num].county_subdivision);
        printf("    %d)county_code (%d) \n", County_Code_num+1, scte_eas_data->location_code_data[County_Code_num].county_code);
        County_Code_num++;
        substart_byte += 3;
    }
    scte_eas_data->exception_count = EAS_payload_start[substart_byte];
    substart_byte += 1;
    //printf("Exception (%d) \n",scte_eas_data->exception_count);
    printf("Exception>\n");
    for(i=0; i< scte_eas_data->exception_count; i++)
    {
        scte_eas_data->exception_data[Exception_num].in_band_reference = EAS_payload_start[substart_byte] >> 7;
        if(scte_eas_data->exception_data[Exception_num].in_band_reference)
        {
            scte_eas_data->exception_data[Exception_num].exception_major_channel_number =  ( (EAS_payload_start[substart_byte+1] << 8) | EAS_payload_start[substart_byte+2]) & DETAIL_CHANNEL_MASK;
            scte_eas_data->exception_data[Exception_num].exception_minor_channel_number = ((EAS_payload_start[substart_byte+3] << 8) | EAS_payload_start[substart_byte+4]) & DETAIL_CHANNEL_MASK;
            printf("    %d)exception_major_channel_number (%d) \n", Exception_num+1, scte_eas_data->exception_data[Exception_num].exception_major_channel_number);
            printf("    %d)exception_minor_channel_number (%d) \n", Exception_num+1, scte_eas_data->exception_data[Exception_num].exception_minor_channel_number);
        }
        else
        {
            scte_eas_data->exception_data[Exception_num].exception_OOB_source_ID = (EAS_payload_start[substart_byte+3] << 8) | EAS_payload_start[substart_byte+4];
            printf("    %d)exception_OOB_source_ID (0x%x) \n", Exception_num+1, scte_eas_data->exception_data[Exception_num].exception_OOB_source_ID);    
        }
        substart_byte += 5;
        Exception_num++;
    }
    scte_eas_data->descriptors_length = ( (EAS_payload_start[substart_byte] << 8) | EAS_payload_start[substart_byte+1] ) & DESCRIPTOR_COUNT_MASK;
    descriptor_num = scte_eas_data->descriptors_length;
    substart_byte += 2;
    crc_start = substart_byte + descriptor_num;
    printf("outer descriptor_num (%d) \n", scte_eas_data->descriptors_length);
    j=0;
    //reserve for descriptor
    while(descriptor_num > 0)
    {
        descriptor_tag = EAS_payload_start[substart_byte];
        descriptor_len = EAS_payload_start[substart_byte+1];
        switch(descriptor_tag)
        {
            case 0x00:
                    printf("        %d)descriptor tag (In-Band Details Channel Descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    printf("        %d)details_RF_channel (%d)\n", j+1, EAS_payload_start[substart_byte+2]);
                    printf("        %d)details_program_number (%d)\n", j+1, ((EAS_payload_start[substart_byte+3] << 8)| EAS_payload_start[substart_byte+4]));
                    break;
            case 0x01:
                    printf("        %d)descriptor tag (In-Band Exceptions Channel Descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    exception_descriptor_count = EAS_payload_start[substart_byte+2];
                    for(i=0; i<exception_descriptor_count;i+=3)
                    {
                        printf("        %d)exception_RF_channel (%d)\n", j+1, EAS_payload_start[substart_byte+3+i]);
                        printf("        %d)exception_program_number (%d)\n", j+1, (EAS_payload_start[substart_byte+4+i] << 8 ) | EAS_payload_start[substart_byte+5+i]);
                    }
                    break;
            case 0x02:
                    printf("        %d)descriptor tag (Audio File Descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    break;
            case 0xAD:
                    printf("        %d)descriptor tag (ATSC Private Information Descriptor)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    break;
            default:
                if(descriptor_tag> 0xc0 && descriptor_tag < 0xff)
                {
                    int xy=0;
                    printf("        %d)descriptor tag (User private descriptors)\n", j+1);
                    printf("        %d)descriptor length (%d)\n", j+1, descriptor_len);
                    for(xy=0; xy< descriptor_len; xy++)
                    {
                        printf("(0x%x)", EAS_payload_start[substart_byte+2+xy]);
                    }
                    printf("\n");
                }
                break;
        }
        descriptor_num = (descriptor_num - descriptor_len - 2);
        substart_byte = (substart_byte + descriptor_len + 2);
        j++;
    }
    //scte_eas_data->CRC = (EAS_payload_start[substart_byte] << 24) | (EAS_payload_start[substart_byte+1] << 16 ) | (EAS_payload_start[substart_byte+2] << 8) | EAS_payload_start[substart_byte+3];
    scte_eas_data->CRC = (EAS_payload_start[crc_start] << 24) | (EAS_payload_start[crc_start+1] << 16 ) | (EAS_payload_start[crc_start+2] << 8) | EAS_payload_start[crc_start+3];
    printf("CRC32 (0x%x) \n", scte_eas_data->CRC);
    //printf("outside byte (0x%x) (0x%x)\n", EAS_payload_start[crc_start+5], EAS_payload_start[crc_start+6]);
    printf("================================\n");
    return 0;
}

/*********************************
*             MPEG-2 Generic function           *
*********************************/

static void TS_Header_Dump(TSPK_HEADER *ts_data)
{
    int i;
   if(ts_data != NULL)
   {
        printf("=============TS HEADER===============\n");
        printf("transport_error_indicator (%x) \n", ts_data->error_bit);
        printf("playload_unit_start_indicator (%x) \n", ts_data->playload_start_bit);
        printf("transport_priority (%x) \n", ts_data->priority_bit);
        printf("PID (%04x) \n", ts_data->PID);
        printf("transport_scrambling_control (%d)/(%s) \n", ts_data->scramble_control, transport_scramble_control[ts_data->scramble_control]);
        printf("adaption_field_control (%x) \n", ts_data->adaption_field_control);
        printf("continuity_counter (%x) \n", ts_data->continuity_counter);
        printf("================================\n"); 
    } 
}

static void PAT_DUMP(TSPK_PAT *pat_data)
{
    int i;
   if(pat_data != NULL)
   {
        printf("==============PAT===============\n");
        printf("PAT_PID (0) \n");
        printf("table_id (0x%x) \n",pat_data->table_id);
        printf("section_lenth (%d) \n",pat_data->section_lenth);
        printf("stream_id (%d) \n",pat_data->stream_id);
        printf("version_number (%d) \n",pat_data->version_number);
        printf("current_next_indicator (%d) \n",pat_data->next_indicator);
        printf("section_number (%d) \n",pat_data->section_number);
        printf("last_section_number (%d) \n",pat_data->last_section_number);
        printf("PMT>\n");
        for(i=0; i<pat_data->PMT_Num;i++){  
            printf("    %d)program number(%d) ", i+1,pat_data->program_number[i]);
            printf("    PMT PID (0x%x) \n", pat_data->PMT_PID[i]);
        }
        printf("CRC32 (0x%x) \n", pat_data->CRC);
        printf("================================\n"); 
    } 
}

static void CAT_DUMP(TSPK_CAT *cat_data)
{
    int i;
   if(cat_data != NULL)
   {
       printf("==============CAT===============\n");
       printf("CAT_PID (0x1) \n");
       printf("table_id (0x%x) \n",cat_data->table_id);
       printf("section_lenth (%d) \n",cat_data->section_lenth);
       printf("version_number (%d) \n",cat_data->version_number);
       printf("current_next_indicator (%d) \n",cat_data->next_indicator);
       printf("section_number (%d) \n",cat_data->section_number);
       printf("last_section_number (%d) \n",cat_data->last_section_number);
       printf("outer descriptor number (%d) \n",cat_data->outer_descriptor_num);
       printf("CRC32 (0x%x) \n", cat_data->CRC);
       printf("================================\n"); 
    } 
}

static void TSDT_DUMP(TSPK_TSDT *tsdt_data)
{
    int i;
   if(tsdt_data != NULL)
   {
       printf("==============TSDT===============\n");
       printf("TSDT_PID (0x2) \n");
       printf("table_id (0x%x) \n",tsdt_data->table_id);
       printf("section_lenth (%d) \n",tsdt_data->section_lenth);
       printf("version_number (%d) \n",tsdt_data->version_number);
       printf("current_next_indicator (%d) \n",tsdt_data->next_indicator);
       printf("section_number (%d) \n",tsdt_data->section_number);
       printf("last_section_number (%d) \n",tsdt_data->last_section_number);
       printf("outer descriptor number (%d) \n",tsdt_data->outer_descriptor_num);
       printf("CRC32 (0x%x) \n", tsdt_data->CRC);
       printf("================================\n"); 
    } 
}

static bool TSPAT_VerChk (TSPK_PAT *pat_data, uchar *PAT_payload)
{
        uchar new_version;
        uchar next_indicator;
        bool CMP = true;
        if(pat_data == NULL || PAT_payload == NULL)
        {
            perror("PAT input data error\n");
            return CMP;
        }
        new_version = ( (PAT_payload[5] >> 1) & VERSION_MASK);
        next_indicator = PAT_payload[5] & CURRENT_NEXT_MASK;
        if(new_version > pat_data->version_number && next_indicator)
        {
            printf("PAT new_version = %d, old_version = %d \n",new_version,  pat_data->version_number);
            CMP = false;
        }
        return CMP;
}

static bool TSCAT_VerChk (TSPK_CAT *cat_data, uchar *CAT_payload)
{
        uchar new_version;
        uchar next_indicator;
        bool CMP = true;
        if(cat_data == NULL || CAT_payload == NULL)
        {
            perror("CAT input data error\n");
            return CMP;
        }
        new_version = ( (CAT_payload[5] >> 1) & VERSION_MASK);
        next_indicator = CAT_payload[5] & CURRENT_NEXT_MASK;
        if(new_version > cat_data->version_number && next_indicator)
        {
            printf("CAT new_version = %d, old_version = %d \n",new_version,  cat_data->version_number);
            CMP = false;
        }
        return CMP;
}

static bool TSDT_VerChk (TSPK_TSDT *tsdt_data, uchar *TSDT_payload)
{
        uchar new_version;
        uchar next_indicator;
        bool CMP = true;
        if(tsdt_data == NULL || TSDT_payload == NULL)
        {
            perror("TSDT input data error\n");
            return CMP;
        }
        new_version = ( (TSDT_payload[5] >> 1) & VERSION_MASK);
        next_indicator = TSDT_payload[5] & CURRENT_NEXT_MASK;
        if(new_version > tsdt_data->version_number && next_indicator)
        {
            printf("TSDT new_version = %d, old_version = %d \n",new_version,  tsdt_data->version_number);
            CMP = false;
        }
        return CMP;
}

static bool compare_pmt (ushort pmt_pid, uchar *PMT_payload)
{
        TSPK_PMT *pmt_list, *pre_server_list, *next_pmt_list;
        uchar *PMT_payload_start = PMT_payload;
        uchar new_version;
        uchar next_indicator;
        bool  found = false;


        if(ex_PMT_list == NULL || PMT_payload == NULL)
        {
            //printf("ex_PMT_list == NULL\n");
            //perror(" incorrect pmt payload or pmt list\n");
            return found;
        }
        //printf("ex_PMT_list != NULL \n");
        new_version = (PMT_payload_start[5]>>1) & VERSION_MASK;
        next_indicator = PMT_payload_start[5] & CURRENT_NEXT_MASK;
        if(!next_indicator)
        {
            found = true;
            return found;
        }
        pmt_list = ex_PMT_list;
        while (pmt_list)
        {
                #if 0
                printf(" @@pmt_pid  (0x%x) \n", pmt_pid);
                printf(" @@pmt_list ->PMT_PID  (0x%x) \n", pmt_list->PMT_PID);
                printf(" @@pmt_list ->Version_number  (%d) \n", pmt_list->version_number);
                #endif
                if(pmt_list->PMT_PID == pmt_pid )
                {
                    //we support version update
                    if(new_version > pmt_list->version_number)
                    {
                         printf("PMT version UP\n");
                         printf("new_version = %d, old_version = %d \n",new_version,  pmt_list->version_number);
                         if(ex_PMT_list->PMT_PID == pmt_pid)
                         {
                             //printf("remove head!!!\n");
                             next_pmt_list = ex_PMT_list->next;
                             free(pmt_list);
                             ex_PMT_list = next_pmt_list;
                         }
                         else
                         {
                            //remove if not head
                            //printf("remove list !!!\n");
                            next_pmt_list = pmt_list->next;
                            free(pmt_list);
                            pre_server_list->next = next_pmt_list;
                         }
                         //support dynamic change
                         VCM_Table_ready = false;
                         VCM_overflow = false;
                         DCM_Table_ready = false;
                         DCM_overflow = false;
                         ICM_Table_ready = false;
                         ICM_overflow = false;
                         CDS_Table_ready = false;
                         CDS_overflow = false;
                         MMS_Table_ready = false;
                         MMS_overflow = false;
                         SNS_Table_ready = false;
                         SNS_overflow = false;
                         found = false;
                         return found;
                    }
                    found = true;
                    return found;
                }
                pre_server_list = pmt_list;
                pmt_list = pmt_list->next ;         
         }
         return found;
}

static void remove_pmt_list(void)
{
    TSPK_PMT *pmt_list;
    if(ex_PMT_list == NULL)
    {
        //printf("ex_PMT_list == NULL\n");
        //perror("PMT list has already empty\n");
        return;
    }
    pmt_list = ex_PMT_list;
    while (pmt_list)
    {
        #if 0
        printf("@@ free--->pmt_list PID (0x%x) \n", pmt_list->PMT_PID);
        #endif
        free(pmt_list);
        pmt_list = pmt_list->next ;         
    }
    ex_PMT_list = NULL;
}

static void TS_structure_show(void)
{
      printf("----------------------------------------\n");
      printf("\n-Transport structure\n");
      printf("                   |_MPEG-2/PSI\n");
      if(PAT_Table_ready)
        printf("                   |           |_PAT \n     ");
      if((PMT_Table_ready) && ((CAT_Table_ready) || (TSDT_Table_ready)))  
        printf("              |           |    |_PMT  \n");
      else
        printf("              |                |_PMT  \n");     
      if(CAT_Table_ready)  
        printf("                   |           |_CAT \n");
      if(TSDT_Table_ready)  
        printf("                   |           |_TSDT \n");
      printf("                   |_SCTE65/PSI\n");
      if(CDS_Table_ready)  
        printf("                   |           |_NIT-CDS        \n");
      if(MMS_Table_ready)  
        printf("                   |           |_NIT-MMS        \n");
      if(SNS_Table_ready)  
        printf("                   |           |_NTT-SNS        \n");
      if(DCM_Table_ready)  
        printf("                   |           |_SVCT-DCM        \n");
      if(VCM_Table_ready)  
        printf("                   |           |_SVCT-VCM        \n");
      if(ICM_Table_ready)  
        printf("                   |           |_SVCT-ICM        \n");  
      if(STT_Table_ready)  
        printf("                   |           |_STT        \n");
        printf("                   |_SCTE18/PSI\n");
      if(INBAND_EAS_Table_ready || OOB_EAS_Table_ready)
        printf("                               |_EAS        \n");
      printf("----------------------------------------\n");  
}

static bool TSPK_OverflowChk(TSPK_HEADER *tspk_data, uchar *TSPK_payload, uchar *ReSem_buf_start, uchar pointer_field, uchar adaption_field_len)
{
    uchar table_id = TSPK_payload[0];
    
    if(tspk_data->PID == 0x1ffc || tspk_data->PID == 0x1ffb) //Parse SCTE
    {
        section_len = (TSPK_payload[1] << 8 | TSPK_payload[2]) & 0xfff; //acutal section len
        section_num = 188 - 8 - pointer_field - adaption_field_len; // 4, 3, 1(pointer field) //standard section number
        
        if(section_num == 0)
            return false;
        
        section_count = (section_len/section_num) + 1;
        
        if(section_count > 1)
        {
            if(table_id == 0xc2)  //NIT
            {
                 if( (TSPK_payload[6] & SUBTYPE_MASK) == 1)
                 {
                     //CDS
                    if(!CDS_Table_ready)
                    {
                         copy_len_oob = section_num + 3;
                         //printf("we got CDS overflow\n");
                         memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                         memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                         ReSem_buf_oob_ptr += copy_len_oob;
                         section_count--;
                         CDS_overflow = true;
                         return true;
                    }
                 }
                 else if((TSPK_payload[6] & SUBTYPE_MASK) == 2)
                 {
                     //MMS
                    if(!MMS_Table_ready)
                    {
                         copy_len_oob = section_num + 3;
                         //printf("we got MMS overflow\n");
                         memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                         memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                         ReSem_buf_oob_ptr += copy_len_oob;
                         section_count--;
                         MMS_overflow = true;
                         return true;
                    }
                 }
            }
            else if(table_id == 0xc3)  //NTT
            {
                //SNS
                 if(!SNS_Table_ready)
                 {
                     copy_len_oob = section_num + 3;
                     //printf("we got SNS overflow\n");
                     memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                     memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                     ReSem_buf_oob_ptr += copy_len_oob;
                     section_count--;
                     SNS_overflow = true;
                     return true;
                 }
            }
            else if(table_id == 0xc4) //SVCT
            {
                 if( (TSPK_payload[4] & SUBTYPE_MASK) == 0)
                 {
                     //VCM
                     if(!VCM_Table_ready)
                     {
                         copy_len_oob = section_num + 3;
                         //printf("we got VCM overflow\n");
                         memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                         memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                         ReSem_buf_oob_ptr += copy_len_oob;
                         section_count--;
                         VCM_overflow = true;
                         return true;
                     }
                 }
                 else if((TSPK_payload[4] & SUBTYPE_MASK) == 1)
                 {
                     //DCM
                     if(!DCM_Table_ready)
                     {
                         copy_len_oob = section_num + 3;
                         //printf("we got DCM overflow\n");
                         memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                         memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                         ReSem_buf_oob_ptr += copy_len_oob;
                         section_count--;
                         DCM_overflow = true;
                         return true;
                     }
                 }
                 else if((TSPK_payload[4] & SUBTYPE_MASK) == 2)
                 {
                     //ICM
                     if(!ICM_Table_ready)
                     {
                         copy_len_oob = section_num + 3;
                         //printf("we got ICM overflow\n");
                         memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                         memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                         ReSem_buf_oob_ptr += copy_len_oob;
                         section_count--;
                         ICM_overflow = true;
                         return true;
                     }
                 }
            }
            else if(table_id == 0xc5)  //STT
            {
                 if(!STT_Table_ready)
                 {
                     copy_len_oob = section_num + 3;
                     //printf("we got STT overflow\n");
                     memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                     memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_oob);
                     ReSem_buf_oob_ptr += copy_len_oob;
                     section_count--;
                     STT_overflow = true;
                     return true;
                }
            }
            else if(table_id == 0xd8)   
            {
                 //EAS
                 if(tspk_data->PID == 0x1ffb)
                 {
                    if(!INBAND_EAS_Table_ready)
                    {
                        copy_len_eas = section_num + 3;
                        //printf("we got EAS overflow\n");
                        memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                        memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_eas);
                        ReSem_buf_eas_ptr += copy_len_eas;
                        section_count--;
                        INBAND_EAS_overflow = true;
                        return true;
                    }
                 }else if(tspk_data->PID == 0x1ffc)
                 {
                    if(!OOB_EAS_Table_ready)
                    {
                        copy_len_eas = section_num + 3;
                        //printf("we got OOB EAS overflow\n");
                        memset(&ReSem_buf_start[0], 0xff, NORMAL_SECTION_LENGTH);
                        memcpy(&ReSem_buf_start[0], &TSPK_payload[0], copy_len_eas);
                        ReSem_buf_eas_ptr += copy_len_eas;
                        section_count--;
                        OOB_EAS_overflow = true;
                        return true;
                    }
                 }
            }
        }
    }
    return false;
}

//Start point
int main(int argc, char *argv[])
{
	FILE *ts_in;
	char *stream_file;
	uchar TSPK_buf[TSPKLEN];
	uint TSPK_header_byte;
      TSPK_HEADER *TSPK_data;
      TSPK_PAT *TSPK_PAT_data;
      TSPK_CAT *TSPK_CAT_data;
      TSPK_TSDT *TSPK_TSDT_data;
      SCTE_NIT_CDS  *NIT_CDS_data;
      SCTE_NIT_MMS *NIT_MMS_data;
      SCTE_NTT_SNS *NTT_SNS_data;
      SCTE_STT *SCTE_STT_data;
      SCTE_SVCT_DCM *SVCT_DCM_data;
      SCTE_SVCT_VCM *SVCT_VCM_data;
      SCTE_SVCT_ICM *SVCT_ICM_data;
      SCTE_EAS *SCTE_EAS_data;
      uchar OOB_buf[NORMAL_SECTION_LENGTH];
      uchar EAS_buf[NORMAL_SECTION_LENGTH];
      bool Chk_result = false;
      int packet_startloc = 4;
	short pointer_field = 0;
	uchar adaption_field_len = 0;
	uchar continuity_counter_oob_old = 0;
	uchar continuity_counter_eas_old = 0;
	ushort VCM_VCT_ID_list[256];
	ushort DCM_VCT_ID_list[256];
	ushort ICM_VCT_ID_list[256];
	uchar VCM_VCT_ID_CurNum = 0;
	uchar DCM_VCT_ID_CurNum = 0;
	uchar ICM_VCT_ID_CurNum = 0;
	bool found_VCT = false;
	int i;
	if(argc < 2)
	{
		printf("please specify TS file\n[USAGE]: TSParser [ts file] > output_result.psi \n[ts file]: input transport stream file\n");
		return 1;
	}
	stream_file = argv[1];
	if(strcmp(stream_file, "--help") ==0 || strcmp(stream_file, "-help") ==0)
	{
	    printf("[USAGE]: TSParser [ts file] > output_result.psi \n[ts file]: input transport stream file\n");
	    return 1;
	}
	if ( (ts_in = fopen(stream_file, "r")) == NULL)
	{
	    printf("TS file open failed (%s)\n", stream_file);
	    return 1;
	}
	TSPK_data = (TSPK_HEADER *)malloc(sizeof(TSPK_HEADER));
	TSPK_PAT_data = (TSPK_PAT *)malloc(sizeof(TSPK_PAT));
	TSPK_CAT_data = (TSPK_CAT *)malloc(sizeof(TSPK_CAT));
	TSPK_TSDT_data = (TSPK_TSDT *)malloc(sizeof(TSPK_TSDT));
	NIT_CDS_data = (SCTE_NIT_CDS *)malloc(sizeof(SCTE_NIT_CDS));
	NIT_MMS_data = (SCTE_NIT_MMS *)malloc(sizeof(SCTE_NIT_MMS));
	NTT_SNS_data = (SCTE_NTT_SNS *)malloc(sizeof(SCTE_NTT_SNS));
	SCTE_STT_data = (SCTE_STT *)malloc(sizeof(SCTE_STT));
	SVCT_DCM_data = (SCTE_SVCT_DCM *)malloc(sizeof(SCTE_SVCT_DCM));
	SVCT_VCM_data = (SCTE_SVCT_VCM *)malloc(sizeof(SCTE_SVCT_VCM));
	SVCT_ICM_data = (SCTE_SVCT_ICM *)malloc(sizeof(SCTE_SVCT_ICM));
	SCTE_EAS_data = (SCTE_EAS *)malloc(sizeof(SCTE_EAS));
	memset(OOB_buf, 0xff, NORMAL_SECTION_LENGTH);
	memset(EAS_buf, 0xff, NORMAL_SECTION_LENGTH);
	while(fread(TSPK_buf, sizeof(unsigned char), TSPKLEN, ts_in))
	{
	    //Sync byte should be 0x47
	    if(TSPK_buf[0] == SYNCBYTE)
	    {      
		    TSPK_header_byte = (TSPK_buf[0] << 24) | (TSPK_buf[1] << 16) | (TSPK_buf[2] << 8) | (TSPK_buf[3]);
		    if(!TSPKHead_Parser(TSPK_data, TSPK_header_byte))
		    {
                      packet_startloc = 4;
                      adaption_field_len = 0;	
                      pointer_field = 0;	        
        	         switch(TSPK_data->adaption_field_control)
        	         {
        	              case  0x3:
        		                //printf("adaption field with payload, len (%d)\n", TSPK_buf[4]+1);
        		                adaption_field_len = TSPK_buf[4]+1;
        		                //reserve for adaption_field
        		                //break;
        		        case  0x1:        		                
        		                //printf("no adaption field but payload\n");
        		                //printf("packet_startloc ==> (%d) \n",packet_startloc);
        		                if(TSPK_data->playload_start_bit)    //1:has pointer field, 0:no pointer field
        		                {
        		                    packet_startloc += adaption_field_len;
        		                    pointer_field = TSPK_buf[packet_startloc];
        		                    if(pointer_field != 0)
        		                    {
        		                         //printf("pointer_field has bytes (%d)\n", pointer_field);
        		                         packet_startloc += pointer_field;
        		                    }
        		                }
        		                else
        		                {
        		                    //printf("no playload_start_bit\n");
        		                    pointer_field = -1;
        		                    packet_startloc = 3+adaption_field_len; //no pointer field;
        		                }   
        		                //printf("## packet_startloc ==> (%d) \n",packet_startloc);
        		                #ifdef TSHEADERDUMP
        		                    TS_Header_Dump(TSPK_data);
        		                #endif
        		                
        		                //Detect dynamic VCT change
        		                #if 1
        		                if(!VCT_detect_lock)
        		                {
        		                    if(TSPK_data->PID == 0x1ffc)
        		                    {
        		                        if(TSPK_buf[packet_startloc+1] == 0xc4)
        		                        {
        		                            i=0;
        		                            found_VCT = false;
        		                            if((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 0)
        		                            {
        		                                //VCM
        		                                if(VCM_VCT_ID_CurNum == 0)
        		                                {
        		                                    VCM_VCT_ID_list[VCM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                    VCM_VCT_ID_CurNum++;
        		                                }
        		                                else
        		                                {
        		                                    while(i <  VCM_VCT_ID_CurNum)
        		                                    {
        		                                        if (VCM_VCT_ID_list[i] == ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])))
        		                                        {
                                                                found_VCT = true;
                                                                break;
        		                                        }
        		                                        i++;
        		                                    }
        		                                    if(!found_VCT)
        		                                    {
        		                                        VCM_VCT_ID_list[VCM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                        VCM_VCT_ID_CurNum++;
        		                                        VCM_Table_ready = false;
                                                           VCM_overflow = false;
                                                           printf("#      < Find VCM VCT_ID(0x%x) >      #\n", ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])));
        		                                    }
        		                                }
        		                            }
        		                            else if((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 1)
        		                            {
        		                                //DCM
        		                                if(DCM_VCT_ID_CurNum == 0)
        		                                {
        		                                    DCM_VCT_ID_list[DCM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                    DCM_VCT_ID_CurNum++;
        		                                }
        		                                else
        		                                {
        		                                    while(i <  DCM_VCT_ID_CurNum)
        		                                    {
        		                                        if (DCM_VCT_ID_list[i] == ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])))
        		                                        {
                                                                found_VCT = true;
                                                                break;
        		                                        }
        		                                        i++;
        		                                    }
        		                                    if(!found_VCT)
        		                                    {
        		                                        DCM_VCT_ID_list[DCM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                        DCM_VCT_ID_CurNum++;
        		                                        DCM_Table_ready = false;
                                                           DCM_overflow = false;
                                                           printf("#      < Find DCM VCT_ID(0x%x) >      #\n", ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])));
        		                                    }
        		                                }
        		                            }
        		                            else if((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 1)
        		                            {
        		                                //ICM
        		                                if(ICM_VCT_ID_CurNum == 0)
        		                                {
        		                                    ICM_VCT_ID_list[ICM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                    ICM_VCT_ID_CurNum++;
        		                                }
        		                                else
        		                                {
        		                                    while(i <  ICM_VCT_ID_CurNum)
        		                                    {
        		                                        if (ICM_VCT_ID_list[i] == ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])))
        		                                        {
                                                                found_VCT = true;
                                                                break;
        		                                        }
        		                                        i++;
        		                                    }
        		                                    if(!found_VCT)
        		                                    {
        		                                        ICM_VCT_ID_list[ICM_VCT_ID_CurNum] = ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7]));
        		                                        ICM_VCT_ID_CurNum++;
        		                                        ICM_Table_ready = false;
                                                           ICM_overflow = false;
                                                           printf("#      < Find ICM VCT_ID(0x%x) >      #\n", ((TSPK_buf[packet_startloc+6] << 8) | (TSPK_buf[packet_startloc+7])));
        		                                    }
        		                                }
        		                            }
                                           }
                                       }
                                   }
                                   #endif
                                    //Table resssemble
        		                #if 1
        		                if( (SNS_overflow && !SNS_Table_ready) || (INBAND_EAS_overflow && !INBAND_EAS_Table_ready) || (OOB_EAS_overflow && !OOB_EAS_Table_ready) || (CDS_overflow && !CDS_Table_ready) || (MMS_overflow && !MMS_Table_ready) || (VCM_overflow && !VCM_Table_ready) || (DCM_overflow && !DCM_Table_ready) || (ICM_overflow && !ICM_Table_ready) || (STT_overflow && !STT_Table_ready))
        		                {
        		                    if(TSPK_data->PID == 0x1ffc)
        		                    {
        		                          if(INBAND_EAS_overflow)
        		                             continue;
                		                    //printf(" OOB next pid (0x%x) table id (0x%x)\n", TSPK_data->PID, TSPK_buf[packet_startloc+1]);
                		                    if(TSPK_data->continuity_counter = (continuity_counter_oob_old + 1))
                		                    {
                		                        if(section_count > 0 )
                		                        {
                		                            uchar temp_len = 188 - 5 - pointer_field - adaption_field_len; // 4, x3, 1(pointer field), //standard section number
                		                            //continue memory copy next playload
                		                            copy_len_oob = section_len - copy_len_oob;
                		                            if(copy_len_oob > temp_len)
                		                            {
                		                                //copy_len = 188;
                		                                memcpy(&OOB_buf[ReSem_buf_oob_ptr], &TSPK_buf[packet_startloc+1], temp_len);
                		                                copy_len_oob += temp_len;
                		                                ReSem_buf_oob_ptr += temp_len;        		                            
                		                                continuity_counter_oob_old = TSPK_data->continuity_counter;
                		                                section_count--;
                		                                //printf("more one packet still copy \n");
                		                                continue;
                		                            }
                		                            else
                		                            {
                		                                //printf("ReSem_buf_oob_ptr (%d) copy_len_oob (%d)\n", ReSem_buf_oob_ptr, copy_len_oob);
                		                                memcpy(&OOB_buf[ReSem_buf_oob_ptr], &TSPK_buf[packet_startloc+1], copy_len_oob+10);
                		                                //printf("all packet ressamble finished ---> start decode\n");
                		                            }
                		                        }
                		                        //printf("continuity_counter match!!!!!!!!!!!\n");
                		                    }
                		                    else
                		                    {
                		                        //next payload doesn't match SNS playload left
                		                        printf("continuity_counter did not match !!!!\n");
                		                        SNS_overflow = false;
                		                        CDS_overflow = false;
                		                        MMS_overflow = false;
                		                        STT_overflow = false;
                		                        DCM_overflow = false;
                		                        VCM_overflow = false;
                		                        ICM_overflow = false;
                		                        OOB_EAS_overflow = false;
                		                        continue;
                		                    }
                		               }
                		               else if(TSPK_data->PID == 0x1ffb)
                		               {
        		                          if(OOB_EAS_overflow || SNS_overflow || CDS_overflow || MMS_overflow || VCM_overflow || DCM_overflow || ICM_overflow || STT_overflow)
        		                             continue;
                		                    //INBAND EAS
                		                    //printf(" EAS next pid (0x%x) table id (0x%x)\n", TSPK_data->PID, TSPK_buf[packet_startloc+1]);
                		                    if(TSPK_data->continuity_counter = (continuity_counter_eas_old + 1))
                		                    {
                		                        if(section_count > 0 )
                		                        {
                		                            uchar temp_len = 188 - 5 - pointer_field - adaption_field_len; // 4, x3, 1(pointer field), //standard section number
                		                            //continue memory copy next playload
                		                            copy_len_eas = section_len - copy_len_eas;
                		                            if(copy_len_eas > temp_len)
                		                            {
                		                                //copy_len_eas = 188;
                		                                memcpy(&EAS_buf[ReSem_buf_eas_ptr], &TSPK_buf[packet_startloc+1], temp_len);
                		                                copy_len_eas += temp_len;
                		                                ReSem_buf_eas_ptr += temp_len;        		                            
                		                                continuity_counter_eas_old = TSPK_data->continuity_counter;
                		                                section_count--;
                		                                //printf("more one packet still copy \n");
                		                                continue;
                		                            }
                		                            else
                		                            {
                		                                //printf("ReSem_buf_eas_ptr (%d) copy_len_eas (%d)\n", ReSem_buf_eas_ptr, copy_len_eas);
                		                                memcpy(&EAS_buf[ReSem_buf_eas_ptr], &TSPK_buf[packet_startloc+1], copy_len_eas+10);
                		                                //printf("all packet ressamble finished ---> start decode\n");
                		                            }
                		                        }
                		                        //printf("continuity_counter match!!!!!!!!!!!\n");
                		                    }
                		                    else
                		                    {
                		                        //next payload doesn't match EAS playload left
                		                        printf("continuity_counter did not match !!!!\n");
                		                        INBAND_EAS_overflow = false;
                		                        continue;
                		                    }
                		               }//enhance
        		                }
        		                else
        		                {
        		                    if(TSPK_data->PID == 0x1ffc)
        		                    {
        		                        if (TSPK_OverflowChk(TSPK_data, &TSPK_buf[packet_startloc+1], OOB_buf, pointer_field, adaption_field_len))
        		                        {
        		                            //printf("got SCTE overflow, prepare for next payload #####\n");
        		                            continuity_counter_oob_old = TSPK_data->continuity_counter;
        		                            VCT_detect_lock = true;
        		                            continue;
        		                        }
        		                    }
        		                    else if(TSPK_data->PID == 0x1ffb)
        		                    {
                                           if (TSPK_OverflowChk(TSPK_data, &TSPK_buf[packet_startloc+1], EAS_buf, pointer_field, adaption_field_len))
        		                        {
        		                            //printf("got EAS overflow, prepare for next payload #####\n");
        		                            continuity_counter_eas_old = TSPK_data->continuity_counter;
        		                            VCT_detect_lock = true;
        		                            continue;
        		                        }
        		                   } 
        		                }
        		                #endif
        		                #if 1
                            	       if(TSPK_data->PID == 0) //Parse PAT
                            	       {    	 
                            	            /*Check PAT table Id*/
                            	            if(TSPK_buf[packet_startloc+1] == 0x0) //Hungwei
                            	            {
                            	                 //version check & need to remove all pmt list
                                	           if(PAT_Table_ready)
                                	           {
                                	                if( !TSPAT_VerChk(TSPK_PAT_data, &TSPK_buf[packet_startloc+1])) //5 //Hungwei
                                	                {
                                	                    printf("#      < PAT Version UP >      #\n");
                                	                    PAT_Table_ready = false;
                                	                    remove_pmt_list();
                                	                    //support for dynamic change
                                	                    VCM_Table_ready = false;
                                	                    VCM_overflow = false;
                                	                    DCM_Table_ready = false;
                                	                    DCM_overflow = false;
                                	                    ICM_Table_ready = false;
                                	                    ICM_overflow = false;
                                	                    CDS_Table_ready = false;
                                	                    CDS_overflow = false;
                                	                    MMS_Table_ready = false;
                                	                    MMS_overflow = false;
                                	                    SNS_Table_ready = false;
                                	                    SNS_overflow = false;
                                	                }
                                	           }
                            	                 if(!PAT_Table_ready)
                            	                 {
        		                                  if(!TSPAT_Parser(TSPK_PAT_data, &TSPK_buf[packet_startloc+1])) //Hungwei
        		                                  {
                	                                      if(TSPK_PAT_data->next_indicator == 1)
                            	                         {
                            	                              #ifdef DUMPPAT
                                                                 PAT_DUMP(TSPK_PAT_data);
                                                              #endif
                                	                        PAT_Table_ready = true;
                                	                   }else{
                            	                              perror("Not valid PAT, , Parse error\n");
                            	                              PAT_Table_ready = false;   
                            	                         }
                            	                      }
                            	                      else
                            	                      {
                            	                          perror("Parse PAT error\n");
                            	                          PAT_Table_ready = false;
                            	                      }
            		                              }
            		                         }
                            	       }
                            	       else if(TSPK_data->PID == 0x01) //Parse CAT
                            	       {
                            	           /*Check CAT table Id*/
                            	           if(TSPK_buf[packet_startloc+1] == 0x01) //Hungwei
                            	           {
                            	                if(CAT_Table_ready)
                            	                {
                            	                    if( !TSCAT_VerChk(TSPK_CAT_data, &TSPK_buf[packet_startloc+1])) //5 //Hungwei
                            	                    {
                            	                        printf("#      < CAT Version UP >      #\n");
                            	                        CAT_Table_ready = false;
                            	                    }
                            	                }
                            	                if(!CAT_Table_ready)
                            	                {
        		                                if(!TSCAT_Parser(TSPK_CAT_data, &TSPK_buf[packet_startloc+1])) //Hungwei
        		                                {
                	                                      if(TSPK_CAT_data->next_indicator == 1)
                            	                        {
                            	                             #ifdef DUMPCAT
                                                                CAT_DUMP(TSPK_CAT_data);
                                                             #endif
                                	                       CAT_Table_ready = true;
                                	                  }else{
                                	                       perror("Not valid CAT, Parse error\n");
                                	                       CAT_Table_ready = false;   
                            	                        }
                            	                    }
                            	                    else
                            	                    {
                            	                        perror("Parse CAT error\n");
                            	                        CAT_Table_ready = false;
                            	                    }
            		                             }
            		                        }
                            	       }
                            	       else if(TSPK_data->PID == 0x02) //Parse TSDT
                            	       {
                            	            if(TSPK_buf[packet_startloc+1] == 0x03) //Hungwei
                            	            {
                            	                 if(TSDT_Table_ready)
                            	                 {
                            	                     if( !TSDT_VerChk(TSPK_TSDT_data, &TSPK_buf[packet_startloc+1])) //5
                            	                     {
                            	                         printf("#      < TSDT Version UP >      #\n");
                            	                         TSDT_Table_ready = false;
                            	                     }
                            	                 }
                            	                 if(!TSDT_Table_ready)
                            	                 {
        		                                  if(!TSDT_Parser(TSPK_TSDT_data, &TSPK_buf[packet_startloc+1])) //Hungwei
        		                                  {
                	                                       if(TSPK_TSDT_data->next_indicator == 1)
                	                                       {
                            	                                #ifdef DUMPTSDT
                                                                    TSDT_DUMP(TSPK_TSDT_data);
                                                                #endif
                                	                         TSDT_Table_ready = true;
                                	                    }else{
                                	                         perror("Not valid TSDT, Parse error\n");
                                	                         TSDT_Table_ready = false;   
                                	                    }
                            	                     }
                            	                     else
                            	                     {
                            	                         perror("Parse TSDT error\n");
                            	                         TSDT_Table_ready = false;
                            	                     }
                            	                 }
            		                         }
                            	       }
                            	       else if(TSPK_data->PID == 0x1ffb) //Parse PSIP SCTE18 description
                            	       {
                            	           if(TSPK_buf[packet_startloc+1] == 0xd8 || INBAND_EAS_overflow)
                                           {
                                                //Inband EAS
                                                if(!INBAND_EAS_Table_ready)
                                                {
                                                    if(INBAND_EAS_overflow)
                                                    {
                                                         if(!SCTE_EAS_Parser(SCTE_EAS_data, &EAS_buf[0], TSPK_data->PID))
                                                         {
                                                             INBAND_EAS_Table_ready = true;
                                                             //printf("we processed overflow EAS\n");
                                                         }
                                                         INBAND_EAS_overflow = false;
                                                         ReSem_buf_eas_ptr = 0;
                                                         VCT_detect_lock = false;
                                                    }
                                                    else
                                                    {
                                                        if(!SCTE_EAS_Parser(SCTE_EAS_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                        {
                                                             INBAND_EAS_Table_ready = true;
                                                        }
                                                    }
                                                }
                                           }
                            	       }
                            	       else if(TSPK_data->PID == 0x1ffc) //Parse OOB SCTE65 description
                            	       {
                            	            /*Check SCTE65/18 table Id*/
                            	            if(TSPK_buf[packet_startloc+1] == 0xc2 || CDS_overflow || MMS_overflow) //Hungwei
                                            {
                                                    if( ((TSPK_buf[packet_startloc+7] & SUBTYPE_MASK) == 1) || CDS_overflow) //Hungwei
                                                    {
                                                        //CDS
                                                        if(!CDS_Table_ready)
                                                        {
                                                            if(CDS_overflow)
                                                            {
                                                                 if( !NIT_CDS_Parser(NIT_CDS_data,&OOB_buf[0], TSPK_data->PID) ) //Hungwei
                                                                 {
                                                                        CDS_Table_ready = true;
                                                                        //printf("we processed overflow CDS\n");
                                                                  }
                                                                  CDS_overflow = false;
                                                                  ReSem_buf_oob_ptr = 0;
                                                                  VCT_detect_lock = false;
                                                            }
                                                            else
                                                            {
                                                                 if( !NIT_CDS_Parser(NIT_CDS_data,&TSPK_buf[packet_startloc+1], TSPK_data->PID) ) //Hungwei
                                                                     CDS_Table_ready = true;
                                                            }
                                                        }
                                                    }
                                                    else if(((TSPK_buf[packet_startloc+7] & SUBTYPE_MASK) == 2) || MMS_overflow) //Hungwei
                                                    {
                                                        //MMS
                                                        if(!MMS_Table_ready)
                                                        {
                                                            if(CDS_overflow)
                                                            {
                                                                 if(!NIT_MMS_Parser(NIT_MMS_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                 {
                                                                        MMS_Table_ready = true;
                                                                        //printf("we processed overflow MMS\n");
                                                                 }
                                                                 MMS_overflow = false;
                                                                 ReSem_buf_oob_ptr = 0;
                                                                 VCT_detect_lock = false;
                                                            }
                                                            else
                                                            {
                                                                 if(!NIT_MMS_Parser(NIT_MMS_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                     MMS_Table_ready = true;
                                                            }
                                                        }
                                                    }
                                              }
                                              else if(TSPK_buf[packet_startloc+1] == 0xc3 || SNS_overflow)
                                              {

                                                  //SNS
                                                  if(!SNS_Table_ready)
                                                  {
                                                        if(SNS_overflow)
        		                                    {
                                                            if(!NTT_SNS_Parser(NTT_SNS_data, &OOB_buf[0], TSPK_data->PID))
                                                            {
                                                                SNS_Table_ready = true;
                                                                //printf("we processed overflow SNS\n");
                                                            }
                                                            SNS_overflow = false;
                                                            ReSem_buf_oob_ptr = 0;
                                                            VCT_detect_lock = false;
                                                        }
                                                        else
                                                        {
                                                            if(!NTT_SNS_Parser(NTT_SNS_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                SNS_Table_ready = true;
                                                        }
                                                  }
                                              }
                                              else if(TSPK_buf[packet_startloc+1] == 0xc4 || VCM_overflow || DCM_overflow || ICM_overflow)
                                              {

                                                  if( ((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 0 ) || VCM_overflow)
                                                  {
                                                        //VCM
                                                        if(!VCM_Table_ready)
                                                        {
                                                            if(VCM_overflow)
                                                            {
                                                                if( !SVCT_VCM_Parser(SVCT_VCM_data,&OOB_buf[0], TSPK_data->PID) )
                                                                {
                                                                    VCM_Table_ready = true;
                                                                    //printf("we processed overflow VCM\n");
                                                                }
                                                                VCM_overflow = false;
                                                                ReSem_buf_oob_ptr = 0;
                                                                VCT_detect_lock = false;
                                                            }
                                                            else
                                                            {
                                                                if( !SVCT_VCM_Parser(SVCT_VCM_data,&TSPK_buf[packet_startloc+1], TSPK_data->PID) )
                                                                    VCM_Table_ready = true;        
                                                            }
                                                        }
                                                   }
                                                   else if(((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 1) || DCM_overflow)
                                                   {
                                                        //DCM
                                                        if(!DCM_Table_ready)
                                                        {
                                                             if(VCM_overflow)
                                                            {
                                                                if(!SVCT_DCM_Parser(SVCT_DCM_data, &OOB_buf[0], TSPK_data->PID))
                                                                {
                                                                    DCM_Table_ready = true;
                                                                    //printf("we processed overflow DCM\n");
                                                                }
                                                                DCM_overflow = false;
                                                                ReSem_buf_oob_ptr = 0;
                                                                VCT_detect_lock = false;
                                                            }
                                                            else
                                                            {
                                                                if(!SVCT_DCM_Parser(SVCT_DCM_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                    DCM_Table_ready = true;
                                                            }
                                                        }
                                                   }
                                                   else if(((TSPK_buf[packet_startloc+5] & SUBTYPE_MASK) == 2) || ICM_overflow)
                                                   {
                                                        //ICM
                                                        if(!ICM_Table_ready)
                                                        {
                                                            if(ICM_overflow)
                                                            {
                                                                if(!SVCT_ICM_Parser(SVCT_ICM_data, &OOB_buf[0], TSPK_data->PID))
                                                                {
                                                                    ICM_Table_ready = true;
                                                                    //printf("we processed overflow ICM\n");
                                                                }
                                                                ICM_overflow = false;
                                                                ReSem_buf_oob_ptr = 0;
                                                                VCT_detect_lock = false;
                                                            }
                                                            else
                                                            {
                                                                if(!SVCT_ICM_Parser(SVCT_ICM_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                    ICM_Table_ready = true;    
                                                            }
                                                        }
                                                   }
                                              }
                                              else if(TSPK_buf[packet_startloc+1] == 0xc5 || STT_overflow)
                                              {
                                                  if(!STT_Table_ready)
                                                  {
                                                       if(STT_overflow)
                                                       {   
                                                            if(!SCTE_STT_Parser(SCTE_STT_data, &OOB_buf[0], TSPK_data->PID))
                                                            { 
                                                                STT_Table_ready = true;
                                                                //printf("we processed overflow STT\n");
                                                            }
                                                            STT_overflow = false;
                                                            ReSem_buf_oob_ptr = 0;
                                                            VCT_detect_lock = false;
                                                       }
                                                       else
                                                       {
                                                            if(!SCTE_STT_Parser(SCTE_STT_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                                STT_Table_ready = true;
                                                       }
                                                  }
                                              }
                                              else if(TSPK_buf[packet_startloc+1] == 0xd8 || OOB_EAS_overflow)
                                              {
                                                    //OOB EAS
                                                    if(!OOB_EAS_Table_ready)
                                                    {
                                                        if(OOB_EAS_overflow)
                                                        {
                                                             if(!SCTE_EAS_Parser(SCTE_EAS_data, &OOB_buf[0], TSPK_data->PID))
                                                             {
                                                                 OOB_EAS_Table_ready = true;
                                                                 //printf("we processed overflow EAS\n");
                                                             }
                                                             OOB_EAS_overflow = false;
                                                             ReSem_buf_oob_ptr = 0;
                                                             VCT_detect_lock = false;
                                                        }
                                                        else
                                                        {
                                                            if(!SCTE_EAS_Parser(SCTE_EAS_data, &TSPK_buf[packet_startloc+1], TSPK_data->PID))
                                                            {
                                                                 OOB_EAS_Table_ready = true;
                                                            }
                                                        }
                                                    }
                                              }                                              
                                              else
                                              {
                                                  //printf("Other not supported SCTE table Id (0x%x)\n", TSPK_buf[packet_startloc+1]);
                                              }
                            	       }else{
                            	            //Parse PMT
                                            if(PAT_Table_ready)
                                            {
                                               /*Check PMT table Id*/
                                               if(TSPK_buf[packet_startloc+1] == 0x02)
                                               {
                                                    if(!compare_pmt(TSPK_data->PID, &TSPK_buf[packet_startloc+1]))
                                                    {
                                                        for(i=0;i<TSPK_PAT_data->PMT_Num;i++)
                                                        {
                                                            if(TSPK_PAT_data->PMT_PID[i] == TSPK_data->PID)
                                                            {
                                	                           //printf("GOT PMT table pid (0x%x) \n", TSPK_data->PID);
                                	                           //ex_PMT_list = TSPMT_Parser(ex_PMT_list, &OOB_buf[0], TSPK_data->PID);
                                	                           ex_PMT_list = TSPMT_Parser(ex_PMT_list, &TSPK_buf[packet_startloc+1], TSPK_data->PID);
                                	                           if(ex_PMT_list != NULL)
                                	                               PMT_Table_ready = true;
                                	                      }
                                	                  }
                                	              }
                                	         }
                                            }
                                       }
                                   #endif
        		                break;
        		        case  0x2:
        		                //printf("adaption field but no payload\n");
        		                break;   
        		        default:
        		                printf("reserved for future used by ISO/IEC\n");
        		                break;
                      }
	           }else{
        		   perror("TS header parse error\n");
        		   remove_pmt_list();
        		   free(SCTE_EAS_data);
        		   free(SVCT_ICM_data);
        		   free(SVCT_VCM_data);
        		   free(SVCT_DCM_data);
        		   free(SCTE_STT_data);
        		   free(NTT_SNS_data);
        		   free(NIT_CDS_data);
        		   free(NIT_MMS_data);
        		   free(TSPK_TSDT_data);
        		   free(TSPK_CAT_data);
        		   free(TSPK_PAT_data);
        		   free(TSPK_data);
        		   fclose(ts_in);
        		   return 1;
	           }
          }else{
                perror("error TS packet\n");
                remove_pmt_list();
                free(SCTE_EAS_data);
                free(SVCT_ICM_data);
                free(SVCT_VCM_data);
                free(SVCT_DCM_data);
      		   free(SCTE_STT_data);
      		   free(NTT_SNS_data);
                free(NIT_CDS_data);
                free(NIT_MMS_data);
                free(TSPK_TSDT_data);
                free(TSPK_CAT_data);
                free(TSPK_PAT_data);
                free(TSPK_data);
                fclose(ts_in);	
                return 1;
          }
      }
      TS_structure_show();
      remove_pmt_list();
      free(SCTE_EAS_data);
      free(SVCT_ICM_data);
      free(SVCT_VCM_data);
      free(SVCT_DCM_data);
      free(SCTE_STT_data);
      free(NTT_SNS_data);
      free(NIT_CDS_data);
      free(NIT_MMS_data);
      free(TSPK_TSDT_data);
      free(TSPK_CAT_data);
      free(TSPK_PAT_data);
      free(TSPK_data);
      fclose(ts_in);	
      return 0;
}
