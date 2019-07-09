#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mpeg2_ps.h"

#define PS_HDR_LEN  14
#define SYS_HDR_LEN 18
#define PSM_HDR_LEN 24
#define PES_HDR_LEN 19
#define RTP_HDR_LEN 12
#define RTP_HDR_SIZE 12
#define RTP_VERSION 2
#define PS_PES_PAYLOAD_SIZE 1300

typedef struct {
    fnPsMuxerCb cb;
    void *      userdata;
    char *      pTempEsData;
    int         tempBuffLen;
} Muxer;

typedef struct  
{  
    int i_size;
    int i_data;
    unsigned char i_mask;
    unsigned char *p_data;
} bits_buffer_s;  

#define bits_write(buffer, count, bits)\
{\
    bits_buffer_s *p_buffer = (buffer);\
    int i_count = (count);\
    unsigned long long i_bits = (bits);\
    while( i_count > 0 )\
    {\
        i_count--;\
        if( ( i_bits >> i_count )&0x01 )\
        {\
            p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;\
        }\
        else\
        {\
            p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;\
        }\
        p_buffer->i_mask >>= 1;\
        if( p_buffer->i_mask == 0 )\
        {\
            p_buffer->i_data++;\
            p_buffer->i_mask = 0x80;\
        }\
    }\
}

static int gb28181_make_ps_header(char *pData, unsigned long long s64Scr)
{
    unsigned long long lScrExt = (s64Scr) % 100;    
    s64Scr = s64Scr / 100;
    bits_buffer_s      bitsBuffer;
    bitsBuffer.i_size = PS_HDR_LEN;    
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data =    (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, PS_HDR_LEN);
    bits_write(&bitsBuffer, 32, 0x000001BA);            /*start codes*/
    bits_write(&bitsBuffer, 2,     1);                        /*marker bits '01b'*/
    bits_write(&bitsBuffer, 3,     (s64Scr>>30)&0x07);     /*System clock [32..30]*/
    bits_write(&bitsBuffer, 1,     1);                        /*marker bit*/
    bits_write(&bitsBuffer, 15, (s64Scr>>15)&0x7FFF);   /*System clock [29..15]*/
    bits_write(&bitsBuffer, 1,     1);                        /*marker bit*/
    bits_write(&bitsBuffer, 15, s64Scr & 0x7fff);        /*System clock [29..15]*/
    bits_write(&bitsBuffer, 1,     1);                        /*marker bit*/
    bits_write(&bitsBuffer, 9,     lScrExt&0x01ff);        /*System clock [14..0]*/
    bits_write(&bitsBuffer, 1,     1);                        /*marker bit*/
    bits_write(&bitsBuffer, 22, (255)&0x3fffff);        /*bit rate(n units of 50 bytes per second.)*/
    bits_write(&bitsBuffer, 2,     3);                        /*marker bits '11'*/
    bits_write(&bitsBuffer, 5,     0x1f);                    /*reserved(reserved for future use)*/
    bits_write(&bitsBuffer, 3,     0);                        /*stuffing length*/
    return 0;
}

static int gb28181_make_sys_header(char *pData)
{    
    bits_buffer_s      bitsBuffer;
    bitsBuffer.i_size = SYS_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data =    (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, SYS_HDR_LEN);
    /*system header*/
    bits_write( &bitsBuffer, 32, 0x000001BB);    /*start code*/
    bits_write( &bitsBuffer, 16, SYS_HDR_LEN-6);/*header_length 表示次字节后面的长度，后面的相关头也是次意�?/
    bits_write( &bitsBuffer, 1,     1);            /*marker_bit*/
    bits_write( &bitsBuffer, 22, 50000);        /*rate_bound*/
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/
    bits_write( &bitsBuffer, 6,  1);            /*audio_bound*/
    bits_write( &bitsBuffer, 1,  0);            /*fixed_flag */
    bits_write( &bitsBuffer, 1,  1);            /*CSPS_flag */
    bits_write( &bitsBuffer, 1,  1);            /*system_audio_lock_flag*/
    bits_write( &bitsBuffer, 1,  1);            /*system_video_lock_flag*/
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/
    bits_write( &bitsBuffer, 5,  1);            /*video_bound*/
    bits_write( &bitsBuffer, 1,  0);            /*dif from mpeg1*/
    bits_write( &bitsBuffer, 7,  0x7F);         /*reserver*/
    /*audio stream bound*/
    bits_write( &bitsBuffer, 8,  0xC0);         /*stream_id*/
    bits_write( &bitsBuffer, 2,  3);            /*marker_bit */
    bits_write( &bitsBuffer, 1,  0);            /*PSTD_buffer_bound_scale*/
    bits_write( &bitsBuffer, 13, 512);          /*PSTD_buffer_size_bound*/
    /*video stream bound*/
    bits_write( &bitsBuffer, 8,  0xE0);         /*stream_id*/
    bits_write( &bitsBuffer, 2,  3);            /*marker_bit */
    bits_write( &bitsBuffer, 1,  1);            /*PSTD_buffer_bound_scale*/
    bits_write( &bitsBuffer, 13, 2048);         /*PSTD_buffer_size_bound*/
    return 0;
}

static int gb28181_make_psm_header(char *pData)
{
    
    bits_buffer_s      bitsBuffer;
    bitsBuffer.i_size = PSM_HDR_LEN; 
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data =    (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, PSM_HDR_LEN);//24Bytes
    bits_write(&bitsBuffer, 24,0x000001);    /*start code*/
    bits_write(&bitsBuffer, 8, 0xBC);        /*map stream id*/
    bits_write(&bitsBuffer, 16,18);            /*program stream map length*/ 
    bits_write(&bitsBuffer, 1, 1);            /*current next indicator */
    bits_write(&bitsBuffer, 2, 3);            /*reserved*/
    bits_write(&bitsBuffer, 5, 0);             /*program stream map version*/
    bits_write(&bitsBuffer, 7, 0x7F);        /*reserved */
    bits_write(&bitsBuffer, 1, 1);            /*marker bit */
    bits_write(&bitsBuffer, 16,0);             /*programe stream info length*/
    bits_write(&bitsBuffer, 16, 8);         /*elementary stream map length    is*/
    /*audio*/
    bits_write(&bitsBuffer, 8, 0x90);       /*stream_type*/
    bits_write(&bitsBuffer, 8, 0xC0);        /*elementary_stream_id*/
    bits_write(&bitsBuffer, 16, 0);         /*elementary_stream_info_length is*/
    /*video*/
    bits_write(&bitsBuffer, 8, 0x1B);       /*stream_type*/
    bits_write(&bitsBuffer, 8, 0xE0);        /*elementary_stream_id*/
    bits_write(&bitsBuffer, 16, 0);         /*elementary_stream_info_length */
    /*crc (2e b9 0f 3d)*/
    bits_write(&bitsBuffer, 8, 0x45);        /*crc (24~31) bits*/
    bits_write(&bitsBuffer, 8, 0xBD);        /*crc (16~23) bits*/
    bits_write(&bitsBuffer, 8, 0xDC);        /*crc (8~15) bits*/
    bits_write(&bitsBuffer, 8, 0xF4);        /*crc (0~7) bits*/
    return 0;
}

static int gb28181_make_pes_header(char *pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts)
{
    
    bits_buffer_s      bitsBuffer;
    bitsBuffer.i_size = PES_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data =    (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, PES_HDR_LEN);
    /*system header*/
    bits_write( &bitsBuffer, 24,0x000001);    /*start code*/
    bits_write( &bitsBuffer, 8, (stream_id));    /*streamID*/
    bits_write( &bitsBuffer, 16,(payload_len)+13);    /*packet_len*/ //指出pes分组中数据长度和该字节后的长度和
    bits_write( &bitsBuffer, 2, 2 );        /*'10'*/
    bits_write( &bitsBuffer, 2, 0 );        /*scrambling_control*/
    bits_write( &bitsBuffer, 1, 0 );        /*priority*/
    bits_write( &bitsBuffer, 1, 0 );        /*data_alignment_indicator*/
    bits_write( &bitsBuffer, 1, 0 );        /*copyright*/
    bits_write( &bitsBuffer, 1, 0 );        /*original_or_copy*/
    bits_write( &bitsBuffer, 1, 1 );        /*PTS_flag*/
    bits_write( &bitsBuffer, 1, 1 );        /*DTS_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*ESCR_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*ES_rate_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*DSM_trick_mode_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*additional_copy_info_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*PES_CRC_flag*/
    bits_write( &bitsBuffer, 1, 0 );        /*PES_extension_flag*/
    bits_write( &bitsBuffer, 8, 10);        /*header_data_length*/ 
    
    /*PTS,DTS*/    
    bits_write( &bitsBuffer, 4, 3 );                    /*'0011'*/
    bits_write( &bitsBuffer, 3, ((pts)>>30)&0x07 );     /*PTS[32..30]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,((pts)>>15)&0x7FFF);    /*PTS[29..15]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,(pts)&0x7FFF);          /*PTS[14..0]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 4, 1 );                    /*'0001'*/
    bits_write( &bitsBuffer, 3, ((dts)>>30)&0x07 );     /*DTS[32..30]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,((dts)>>15)&0x7FFF);    /*DTS[29..15]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,(dts)&0x7FFF);          /*DTS[14..0]*/
    bits_write( &bitsBuffer, 1, 1 );
    return 0;
}

static int gb28181_make_rtp_header(char *pData, int marker_flag, unsigned short cseq, long long curpts, unsigned int ssrc)
{
    bits_buffer_s      bitsBuffer;
    if (pData == NULL)
        return -1;
    bitsBuffer.i_size = RTP_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data =    (unsigned char *)(pData);
    
    memset(bitsBuffer.p_data, 0, RTP_HDR_SIZE);
    bits_write(&bitsBuffer, 2, RTP_VERSION);    /* rtp version     */
    bits_write(&bitsBuffer, 1, 0);                /* rtp padding     */
    bits_write(&bitsBuffer, 1, 0);                /* rtp extension     */
    bits_write(&bitsBuffer, 4, 0);                /* rtp CSRC count */
    bits_write(&bitsBuffer, 1, (marker_flag));            /* rtp marker      */
    bits_write(&bitsBuffer, 7, 96);            /* rtp payload type*/
    bits_write(&bitsBuffer, 16, (cseq));            /* rtp sequence      */
    bits_write(&bitsBuffer, 32, (curpts));         /* rtp timestamp      */
    bits_write(&bitsBuffer, 32, (ssrc));         /* rtp SSRC          */
    return 0;
}

static int streampackage_es2ps(Muxer *hd, char *esData, int esLen, int frm_type, long long pts, fnPsMuxerCb cb_func, void *userdata)
{
    int buf_offset;
    char szTempPacketHead[256];
    int  nSizePos = 0;
    int  nSize = 0;        
    char *pBuff = NULL;

    memset(szTempPacketHead, 0, 256);
    buf_offset = 0;
    
    gb28181_make_ps_header(szTempPacketHead + nSizePos, pts);
    nSizePos += PS_HDR_LEN;

    if (frm_type) {
        gb28181_make_sys_header(szTempPacketHead + nSizePos);
        nSizePos += SYS_HDR_LEN;
    }
    gb28181_make_psm_header(szTempPacketHead + nSizePos);
    nSizePos += PSM_HDR_LEN;

    cb_func(szTempPacketHead, nSizePos, 0);
    if (hd->tempBuffLen < esLen + PES_HDR_LEN) {
        if (hd->pTempEsData) {free(hd->pTempEsData);}
        hd->pTempEsData = (char *)malloc(esLen + PES_HDR_LEN);
        if (!hd->pTempEsData) return -1;
    }
    memcpy(hd->pTempEsData + PES_HDR_LEN, esData, esLen);
    pBuff = hd->pTempEsData;

    while(esLen > 0) {  
        nSize = (esLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : esLen;
        gb28181_make_pes_header(pBuff, 0 ? 0xC0:0xE0, nSize, (pts / 100), (pts / 300));  
        cb_func(pBuff, nSize + PES_HDR_LEN, ((nSize == esLen)?1:0)); 
        esLen -= nSize;
        pBuff += nSize;
    }
    return 0;
}

static long long get_timestamp()
{
    long long time_stamp;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_stamp = ((tv.tv_sec * 1000) + (tv.tv_usec/1000));

    return time_stamp;
}

MuxerHandle create_ps_muxer(fnPsMuxerCb muxer_cb, void *user)
{
    Muxer *ht = (Muxer *)malloc(sizeof(Muxer));
    ht->cb = muxer_cb;
    ht->userdata = user;
    ht->pTempEsData = NULL;
    ht->tempBuffLen = 0;
    return (MuxerHandle *)ht;
}

void release_ps_muxer(MuxerHandle* muxer)
{
    Muxer *ht = (Muxer *)muxer;
    if (ht) {
        if (ht->pTempEsData) (free(ht->pTempEsData));
        free(ht);
        ht = NULL;
    }
}

int input_es_frame(MuxerHandle hd, EsFrame *frame)
{
    Muxer *ht = (Muxer *)hd;
    if (!ht) {
        printf("ps muxer Not Initialized, failed! \n");
        return -1;
    }
    
    if (0 != streampackage_es2ps(ht, frame->esRawData, frame->esFrameLen, frame->isKeyFrame, get_timestamp(), ht->cb, ht->userdata)) {
        //log here
        return -1;
    }
    return 0;
}
