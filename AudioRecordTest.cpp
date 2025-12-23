
#define LOG_TAG "AudioRecordTest"

#include <utils/Log.h>
#include <media/AudioRecord.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#include <utils/StopWatch.h>


using namespace android;

 
//==============================================
//	Audio Record Defination
//==============================================

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "AudioRecordTest"
 

static bool exit_flag = false;

void handle_signal(int signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        exit_flag = true;
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
}

bool IsValidVoice(const unsigned char *pcmdata, size_t size)
{
	int db = 0;
	short int value = 0;
	double sum = 0;
	for (int i = 0; i < size; i += 2) {
		memcpy(&value, pcmdata + i, 2);
		sum += abs(value);
	}
	sum = sum / (size / 2);

	if (sum > 0) {
		db = (int)(20.0*log10(sum));
	}

	if (db > 0) {
		return true;
	} else {
		return false;
	}
}

void AudioRecordCallback(int event, void* user, void* info)
{
	(void)user;

	if (event == android::AudioRecord::EVENT_NEW_POS)
	{
		ALOGD("audio EVENT_NEW_POS\n");
	}
	else if (event == android::AudioRecord::EVENT_MORE_DATA)
	{
		android::AudioRecord::Buffer* pBuff = (android::AudioRecord::Buffer*)info;
		pBuff->size = 0;
		ALOGD("audio EVENT_MORE_DATA\n");
	}
	else if (event == android::AudioRecord::EVENT_OVERRUN)
	{
		ALOGD("audio EVENT_OVERRUN \n");
	}
}

static void * AudioRecordThread(int sample_rate, int channels, void *fileName)
{
	void *					inBuffer 		= NULL; 
	audio_source_t 			inputSource 	= AUDIO_SOURCE_REMOTE_SUBMIX;	// AUDIO_SOURCE_MIC
	audio_format_t 		 	audioFormat 	= AUDIO_FORMAT_PCM_16_BIT;	
	audio_channel_mask_t 	channelConfig	= AUDIO_CHANNEL_IN_STEREO;
	int bufferSizeInBytes;
	int sampleRateInHz 						= sample_rate; //8000; //44100;	
	android::AudioRecord *	pAudioRecord 	= NULL;
	FILE * 	g_pAudioRecordFile 				= NULL;
	char * 	strAudioFile 					= (char *)fileName;
 
	int iNbChannels 						= channels;	// 1 channel for mono, 2 channel for streo
	int iBytesPerSample 					= 2; 	// 16bits pcm, 2Bytes
	int frameSize 							= 0;	// frameSize = iNbChannels * iBytesPerSample
    size_t  minFrameCount 					= 0;	// get from AudroRecord object
	int iWriteDataCount 					= 0;	// how many data are there write to file
	
	// log the thread id for debug info
	g_pAudioRecordFile = fopen(strAudioFile, "wb");	
	
	//printf("sample_rate = %d, channels = %d, iNbChannels = %d, channelConfig = 0x%x\n", sample_rate, channels, iNbChannels, channelConfig);
	
	//iNbChannels = (channelConfig == AUDIO_CHANNEL_IN_STEREO) ? 2 : 1;
	if (iNbChannels == 2) {
		channelConfig = AUDIO_CHANNEL_IN_STEREO;
	}
	// printf("sample_rate=%d, channels=%d, iNbChannels=%d, channelConfig=0x%x\n", sample_rate, channels, iNbChannels, channelConfig);
	// ALOGD("sample_rate=%d, channels=%d, iNbChannels = %d, channelConfig = 0x%x\n", sample_rate, channels, iNbChannels, channelConfig);
	
	frameSize 	= iNbChannels * iBytesPerSample;	
	
	android::status_t 	status = android::AudioRecord::getMinFrameCount(
		&minFrameCount, sampleRateInHz, audioFormat, channelConfig);	
	
	if(status != android::NO_ERROR)
	{
		ALOGE("%s  AudioRecord.getMinFrameCount fail \n", __FUNCTION__);
		goto exit;
	}
	
	ALOGD("sampleRate=%d, minFrameCount=%zu, channels=%d, channelConfig=0x%x, frameSize=%d", 
		sampleRateInHz, minFrameCount, iNbChannels, channelConfig, frameSize);

	printf("sampleRate=%d, minFrameCount=%zu, channels=%d, channelConfig=0x%x, frameSize=%d\n", 
		sampleRateInHz, minFrameCount, iNbChannels, channelConfig, frameSize);

	// bufferSizeInBytes = minFrameCount * frameSize;
	bufferSizeInBytes = minFrameCount * 2;
	
	inBuffer = malloc(bufferSizeInBytes); 
	if(inBuffer == NULL)
	{		
		ALOGE("%s  alloc mem failed \n", __FUNCTION__);		
		goto exit; 
	}
 	
	pAudioRecord = new android::AudioRecord(String16("audio_test"));	
	if(NULL == pAudioRecord)
	{
		ALOGE(" create native AudioRecord failed! ");
		goto exit;
	}
	
	pAudioRecord->set(inputSource,
					sampleRateInHz,
					audioFormat,
					channelConfig,
					0,
					AudioRecordCallback,
					NULL,
					0,
					true,
					(audio_session_t)0); 
 
	if(pAudioRecord->initCheck() != android::NO_ERROR)  
	{
		ALOGE("AudioTrack initCheck error!");
		goto exit;
	}
 	
	if(pAudioRecord->start()!= android::NO_ERROR)
	{
		ALOGE("AudioTrack start error!");
		goto exit;
	}	
	
	while (!exit_flag)
	{
		android::StopWatch stopWatch("audio_read_test");
		int readLen = pAudioRecord->read(inBuffer, bufferSizeInBytes);				
		if(readLen > 0) 
		{
			int writeResult = -1;

			if (! IsValidVoice((const unsigned char *)inBuffer, readLen)) {
				// ALOGD("mute unvalid voice\n");
				continue;	
			}

			iWriteDataCount += readLen;
			if(NULL != g_pAudioRecordFile)
			{
				writeResult = fwrite(inBuffer, 1, readLen, g_pAudioRecordFile);				
				if(writeResult < readLen)
				{
					ALOGE("Write Audio Record Stream error");
				}
			}			
 
			ALOGD("readLen = %d  writeResult = %d  iWriteDataCount = %d", readLen, writeResult, iWriteDataCount);			
		}
		else 
		{
			ALOGE("pAudioRecord->read readLen = 0");
		}
	}
		
exit:
	if(NULL != g_pAudioRecordFile)
	{
		fflush(g_pAudioRecordFile);
		fclose(g_pAudioRecordFile);
		g_pAudioRecordFile = NULL;
	}
 
	if(pAudioRecord)
	{
		pAudioRecord->stop();
		// delete pAudioRecord;
		// pAudioRecord == NULL;
	}
 
	if(inBuffer)
	{
		free(inBuffer);
		inBuffer = NULL;
	}
	
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("Usage:\n");
		printf("%s <sample_rate> <channels> <out_file>\n", argv[0]);
		return -1;
	}
	AudioRecordThread(strtol(argv[1], NULL, 0), strtol(argv[2], NULL, 0), argv[3]);

	return 0;
}

