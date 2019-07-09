#ifndef __MPEG2_PS_H__
#define __MPEG2_PS_H__

typedef void * MuxerHandle;

typedef struct {
    int     isKeyFrame;
    int     esFrameLen;
    char *  esRawData;
} EsFrame;

typedef void (*fnPsMuxerCb)(char* psData, unsigned long dataLen, unsigned int lastPackMark);

MuxerHandle create_ps_muxer(fnPsMuxerCb muxer_cb);

void release_ps_muxer(MuxerHandle hd);

int input_es_frame(MuxerHandle hd, EsFrame *frame);

#endif
