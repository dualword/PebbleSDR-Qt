#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include <QMainWindow>
#include <QtCore/QVariant>
#include <QMutex>
#include "audio.h"
#include "audiopa.h"
#include "cpx.h"
#include "demod.h"
#include "mixer.h"
#include "smeterwidget.h"
#include "receiverwidget.h"
#include "settings.h"
#include "presets.h"
#include "signalprocessing.h"
#include "noiseblanker.h"
#include "noisefilter.h"
#include "signalstrength.h"
#include "signalspectrum.h"
#include "firfilter.h"
#include "agc.h"
#include "iqbalance.h"
#include "filters/fir.h"
#include "filters/fractresampler.h"
#include "devices/wavfile.h"
#include "sdroptions.h"
#include "fft.h"

#include "plugins.h"
#include <QMenuBar>

class Receiver:public QObject
{
	Q_OBJECT

public:
	Receiver(ReceiverWidget *rw, QMainWindow *main);
	~Receiver(void);
	bool On();
	bool Off();
	void Close(); //Shutdown, just before destructor
	bool Power(bool on);
	//Called with a requested frequency and the last frequency
	//Calculates next higher or lower (actual), sets it and return to widget for display
	double SetFrequency(double fRequested, double fCurrent);
	void SetMode(DeviceInterface::DEMODMODE m);
	void SetGain(int g);
	void SetAgcThreshold(int g);
	void SetSquelch(int s);
	void SetMixer(int f);
	void SetFilter(int lo, int hi);
	void SetAnfEnabled(bool b);
	void SetNbEnabled(bool b);
	void SetNb2Enabled(bool b);
	int SetAgcMode(AGC::AGCMODE m);
	void SetMute(bool b);
	void ShowPresets();
    Presets *GetPresets() {return presets;}
    bool GetPowerOn() {return powerOn;}

	Settings * GetSettings() {return settings;}
    void ProcessIQData(CPX *in, quint16 numSamples);
	void ProcessBandscopeData(quint8 *in, quint16 numPoints);
	void ProcessAudioData(CPX *in, quint16 numSamples);
	SignalStrength *GetSignalStrength() {return signalStrength;}
	SignalSpectrum *GetSignalSpectrum() {return signalSpectrum;}
	IQBalance *GetIQBalance(){return iqBalance;}

    ReceiverWidget *receiverWidget;

    DigitalModemInterface *getDigitalModem() {return iDigitalModem;}
    void SetDigitalModem(QString _name, QWidget *_parent);
    QList<PluginInfo> getModemPluginInfo() {return plugins->GetModemPluginInfo();}
    QList<PluginInfo> getDevicePluginInfo() {return plugins->GetDevicePluginInfo();}

    Demod *getDemod() {return demod;}


    public slots:
		void Restart();
        void RecToggled(bool on);
        void SdrOptionsPressed();
        void CloseSdrOptions();
        void SetWindowTitle();
		void openTestBench();
		void openAboutBox();
		void openDeviceAboutBox();

private:
    Plugins *plugins;
	SdrOptions *sdrOptions;
	QMenuBar *mainMenu;
	QMenu *developerMenu;
	QMenu *helpMenu;


    //Test bench profiles we can output data to test bench
    enum TestBenchProfiles {
        testBenchRawIQ = 1,
        testBenchPostMixer,
        testBenchPostBandpass,
        testBenchPostDemod
    };


	FFT *fft;
	int fftSize;
	bool useFreqDomainChain;

	bool mute;
	QMutex mutex;
	bool powerOn;
	Settings *settings;
	Presets *presets;
	QMainWindow *mainWindow;
	DeviceInterface *sdr;
	Audio *audioOutput;
	Demod *demod;
	Mixer *mixer;
	NoiseBlanker *noiseBlanker;
	NoiseFilter *noiseFilter;
	SignalStrength *signalStrength;
	SignalSpectrum *signalSpectrum;
	AGC *agc;
	IQBalance *iqBalance;
    DigitalModemInterface *iDigitalModem; //Active digital modem if any
    bool isRecording;
    QString recordingFileName;
    QString recordingPath;
    WavFile recordingFile;

	double frequency; //Current LO frequency (not mixed)
    double mixerFrequency;
	int sampleRate;
	int framesPerBuffer; //#samples in each callback
	//sample rate and buffer size after down sampling step
    int demodFrames;
    int downSample2Frames;
    FIRFilter *downSampleFilter;

    //Trying cuteSdr downsample code
    CFractResampler fractResampler; //To get to final audio rate
    CDownConvert downConvert1; //Get to reasonable rate for demod and following
    CDownConvert downConvertWfm1; //Special to get to 300k
    int audioOutRate;
    int demodSampleRate;
    int demodWfmSampleRate;
    CPX *workingBuf;
    CPX *sampleBuf; //Used to accumulate post mixer/downconvert samples to get a full buffer
    CPX *audioBuf; //Used for final audio output processing
    quint16 sampleBufLen;

	double *dbSpectrumBuf; //Used when spectrum is set by remote

    //Use CuteSDR decimation filters for downsampling
    CDecimateBy2 *decimate1;
    int decimate2SampleRate;
    CDecimateBy2 *decmiate2;
    int decimate3SampleRate;
    CDecimateBy2 *decimate3;
    int decimate4SampleRate;
    CDecimateBy2 *decimate4;

	float gain;
	double sdrGain;
	int squelch;

	FIRFilter *bpFilter; //Current BandPass filter
	FIRFilter *usbFilter;
	FIRFilter *lsbFilter;
	FIRFilter *amFilter;

};
