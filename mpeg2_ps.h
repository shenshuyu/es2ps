#ifndef __MPEG2_PS_H__
#define __MPEG2_PS_H__

typedef void * MuxerHandle;

typedef struct {
    int     isKeyFrame;
    int     esFrameLen;
    char *  esRawData;
} EsFrame;

typedef void (*fnPsMuxerFrameCb)(char* psData, unsigned long dataLen, unsigned int lastPackMark);

/**
 * create handle
 *
 * @param fnPsMuxerFrameCb: callback ps frame
 *					
 * @return muxer handle
 */ 
MuxerHandle CreatePsStreamMuxer(fnPsMuxerFrameCb muxer_cb);

/**
 * Insert h264 (ES) Frame
 *
 * @return success:0, fail:other value
 */ 
int InputEsFrame(MuxerHandle hd, EsFrame *frame);

/**
 * Release converter
 *
 */ 
void ReleasePsMuxer(MuxerHandle hd);

#endif
