//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "soundcard.h"
#include "receiver.h"

/*
  Notes:
  7/14/12: Mac coreaudiod went into high CPU spin, 25% instead of normal 2.5%.
    Problem was corrupt .plist files in Lib/Preferences/Audio
    Delete com.apple.audio.DeviceSettings.plist and com.apple.audio.SystemSettings.plist and restart
    Files should be re-created and problem goes away.

*/
SoundCard::SoundCard(Receiver *r, int fpb, Settings *s):Audio()
{
	settings = s;
	//Float32 format returns values between -1 and +1
	sampleFormat = paFloat32;
	//Todo: These can be removed I think, we're now setting them in Start() where we have SDR context
	framesPerBuffer = fpb;
	receiver=r;
	//Deliver samples to this buffer
	inBuffer = CPXBuf::malloc(framesPerBuffer);
	outBuffer = CPXBuf::malloc(framesPerBuffer);
	outStreamBuffer = new float[framesPerBuffer * 2];
	inStream=NULL;
	outStream=NULL;
	error = Pa_Initialize();
}

SoundCard::~SoundCard(void)
{
	Stop();
	Pa_Terminate();
}

int SoundCard::StartInput(QString inputDeviceName, int _inputSampleRate)
{
    inputSampleRate = _inputSampleRate;
    inBufferUnderflowCount = 0;
    inBufferOverflowCount = 0;
    PaStreamParameters *inParam = new PaStreamParameters();
    //We're getting input from sound card
    //inParam->device = settings->inputDevice;
    inParam->device = FindDeviceByName(inputDeviceName,true);
    if (inParam->device < 0)
        return 0;

    inParam->channelCount = 2;
    inParam->sampleFormat = sampleFormat;
    //Bug: crashes if latency anything except 0, looks like struct is getting corrupted
    //Update: Mac doesn't like zero and only works with device info
    //inParam->suggestedLatency = 0;
    inParam->suggestedLatency = Pa_GetDeviceInfo(inParam->device)->defaultLowInputLatency;  //Todo: Understand what this means
    inParam->hostApiSpecificStreamInfo = NULL;

    error = Pa_IsFormatSupported(inParam,NULL,inputSampleRate);
    if (!error) {
        error = Pa_OpenStream(&inStream,inParam,NULL,
            inputSampleRate,framesPerBuffer,paNoFlag,&SoundCard::streamCallback,this );
        error = Pa_StartStream(inStream);
    }

    return 0;
}

int SoundCard::StartOutput(QString outputDeviceName, int _outputSampleRate)
{
	outputSampleRate = _outputSampleRate;

	outBufferUnderflowCount = 0;
	outBufferOverflowCount = 0;

	PaStreamParameters *outParam = new PaStreamParameters();

    //outParam->device = settings->outputDevice;
    outParam->device = FindDeviceByName(outputDeviceName,false);
    if (outParam->device < 0)
        return 0;

    outParam->channelCount = 2;
    outParam->sampleFormat = sampleFormat;
    //Bug: same as above
    //outParam->suggestedLatency = 0;
    outParam->suggestedLatency = Pa_GetDeviceInfo(outParam->device)->defaultLowOutputLatency;
    outParam->hostApiSpecificStreamInfo = NULL;

    //For performance reasons, we try to keep output sample rate low.
    //Surprisingly, this is a primary issue in higher sample rates, like from SDR-IQ (196k)
    //See if we have an even divisor we can use for simple decimation
    //decimate has to be an even multiple of framesPerBuffer so that when we take every Nth sample and output it,
    //we don't have any any uneven samples left in the buffer.  If so, then when we start with the next buffer,
    //we'll hear a skip or static as we jump to a non-regular skip interval
    //Update: Too much thinking.  Works better if we just find the smallest decimation factor and don't worry
    //about precise end-of-buffer scenarios
#if(0)
    decimate = 1;
    //Decimate moved to post mixer phase, here for reference
    decimate = outputSampleRate / settings->decimateLimit;
    decimate = decimate < 1 ? 1 : decimate;
    outputSampleRate /= decimate;
#endif
    error = Pa_IsFormatSupported(NULL,outParam,outputSampleRate);
    if (!error) {
        //Sample rate is set by SDR device if not using soundCard for I/Q
        //NOTE: PortAudio does strange things if we open stream with a small buffer, even if that's all we need
        //Symptoms are long processing times and garbled audio
        //To avoid this, we always open with value from settings, even if we only use a fraction
        //of the buffer due to downsampling.
        //SendToOutput() will use the actual number of samples in buffer, not the size.
        error = Pa_OpenStream(&outStream,NULL,outParam,
            outputSampleRate,settings->framesPerBuffer,paNoFlag,NULL,NULL );

        error = Pa_StartStream(outStream);
    }
	return 0;
}

int SoundCard::Stop()
{
	if (inStream)
	{
		error = Pa_StopStream(inStream);
		error = Pa_CloseStream(inStream);
	}
	if (outStream){
		error = Pa_StopStream(outStream);
		error = Pa_CloseStream(outStream);
	}
	inStream = NULL;
	outStream = NULL;
	return 0;
}
int SoundCard::Flush()
{
	if (inStream)
		error = Pa_AbortStream(inStream);
	return 0;
}
int SoundCard::Pause()
{
	if (inStream)
		error = Pa_StopStream(inStream);
	if (outStream)
		error = Pa_StopStream(outStream);
	return 0;
}
int SoundCard::Restart()
{
	if (inStream)
		error = Pa_StartStream(inStream);
	if (outStream)
		error = Pa_StartStream(outStream);
	return 0;
}
int SoundCard::DefaultInputDevice()
{
	Pa_Initialize();
	int r = Pa_GetDefaultInputDevice();
	Pa_Terminate();
	return r;
}
int SoundCard::DefaultOutputDevice()
{
	Pa_Initialize();
	int r = Pa_GetDefaultOutputDevice();
	Pa_Terminate();
	return r;
}

int SoundCard::FindDeviceByName(QString name, bool inputDevice)
{
    QString di;
    int apiCnt = Pa_GetHostApiCount();
    const PaHostApiInfo *apiInfo;
    const PaDeviceInfo *devInfo;

    for (int i = 0; i < apiCnt; i++)
    {
        apiInfo = Pa_GetHostApiInfo(i);
        for (int j=0; j < apiInfo->deviceCount; j++){
            devInfo = Pa_GetDeviceInfo(j);
            if ((inputDevice && devInfo->maxInputChannels > 1) || (!inputDevice && devInfo->maxOutputChannels > 0)){
                //Must be EXACT format as returned by DeviceList, minus device index number
                di = di.sprintf("%s:%s",apiInfo->name,devInfo->name);
                if (di == name)
                    return Pa_HostApiDeviceIndexToDeviceIndex(i,j); //Index
            }
        }
    }
    return -1; //Not found
}

//Returns a device list for UI
//paDeviceIndex is returned in 1st 2 char, strip for UI
//Todo: Write .txt file with all device info to help with debugging
QStringList SoundCard::InputDeviceList()
{
    return DeviceList(true);
}
QStringList SoundCard::OutputDeviceList()
{
    return DeviceList(false);
}

QStringList SoundCard::DeviceList(bool typeInput)
{
	//This static method may get called before constructor
	//So we have to make sure Pa_Initialize is called, make sure we have matching Pa_Terminate()
	Pa_Initialize();
	QStringList devList;
	QString di;
	int apiCnt = Pa_GetHostApiCount();
	const PaHostApiInfo *apiInfo;
	const PaDeviceInfo *devInfo;

	for (int i = 0; i < apiCnt; i++)
	{
		apiInfo = Pa_GetHostApiInfo(i);
		for (int j=0; j < apiInfo->deviceCount; j++)
		{
			devInfo = Pa_GetDeviceInfo(j);
			//Note: PortAudio must be compiled as ASIO files to enable ASIO4ALL driver support
            //Ignore device types we don't support
            if (apiInfo->type==paMME
                    || apiInfo->type==paASIO
                    || apiInfo->type==paDirectSound
                    //Mac types
                    || apiInfo->type==paCoreAudio)
            {

                //Ignore input and output devices that only have 1 channel
                //We need stereo in for I/Q
                if(typeInput && devInfo->maxInputChannels > 1)
                {
                    di = di.sprintf("%2d:%s:%s",Pa_HostApiDeviceIndexToDeviceIndex(i,j),
                        apiInfo->name,devInfo->name);
                    devList << di;
                }
                else if(!typeInput && devInfo->maxOutputChannels > 1)
                {
                    //This is the fully qualified name that will be saved and used to find the same device
                    //See FindDeviceByName()
                    di = di.sprintf("%2d:%s:%s",Pa_HostApiDeviceIndexToDeviceIndex(i,j),
                        apiInfo->name,devInfo->name);
                    devList << di;
                }
            }
        }
	}
	Pa_Terminate();
	return devList;
}
void SoundCard::ClearCounts()
{
	inBufferOverflowCount = 0;
	inBufferUnderflowCount = 0;
	outBufferOverflowCount = 0;
	outBufferUnderflowCount = 0;
}
//Left and Right channel samples are interleaved
int SoundCard::streamCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData )
{
	//Soundcard object is passed in userData, get it so we can access member functions
	SoundCard *sc = (SoundCard*)userData;

	//Check for buffer overrun, which means we're getting data faster than we can process it
	if (statusFlags & paInputUnderflow) {
		sc->inBufferUnderflowCount++;
	}
	if (statusFlags & paInputOverflow) {
		sc->inBufferOverflowCount++;
	}
	if (statusFlags & paOutputUnderflow) {
		sc->outBufferUnderflowCount++;
	}
	if (statusFlags & paOutputOverflow){
		sc->outBufferOverflowCount++;
	}
	float *inPtr = (float*)input;

	float I,Q;

	//Testing to see how microphone level and microphone booster affect sound levels
	//This could be captured in a troubleshooting page if there are sound card variations
	/*
		Level/Boost	50%		100%	+10db	+20db	+30db
		maxI & maxQ	0.015	0.022	0.080	0.120	0.680
	*/
	//float maxI = 0;
	//float maxQ = 0;
	//Put interleaved float samples into CPX buffer
	for (unsigned i=0;i<frameCount;i++)
	{
		I = *inPtr++;
		//maxI = I>maxI ? I : maxI;
		Q = *inPtr++;
		//maxQ = Q>maxQ ? Q : maxQ;
		sc->inBuffer[i]=CPX(I,Q);
	}
	//ProcessBlock handles all receive chain and ouput
    sc->receiver->ProcessBlock(sc->inBuffer,sc->outBuffer,frameCount);

	return paContinue;
}
//Final call from receiver ProcessBlock to send audio out
void SoundCard::SendToOutput(CPX *out, int outSamples, float gain, bool mute)
{
	float *outPtr;
	outPtr = outStreamBuffer;
    float temp;

	//We may have to skip samples to reduce rate to match audio out, decimate set when we
	//opened stream
    for (int i=0;i<outSamples;i++)
	{
        if (mute)
            out[i].re = out[i].im = 0;

        if (gain != 1)
            out[i] *= gain;

        temp = out[i].re;
        //Cap at -1 to +1 to make sure we don't overdrive
        if (temp >.9999)
            temp = .9999;
        else if (temp < -.9999)
            temp = -.9999;
        *outPtr++ = temp;
        temp = out[i].im;
        //Cap at -1 to +1 to make sure we don't overdrive
        if (temp >.9999)
            temp = .9999;
        else if (temp < -.9999)
            temp = -.9999;
        *outPtr++ = temp;
	}

	//Note we use frameCount, not #bytes in outStreamBuffer.  WriteStream knows format
    error = Pa_WriteStream(outStream,outStreamBuffer,outSamples);
}

