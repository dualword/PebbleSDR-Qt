/* PebbleSDR-Qt (2020) http://github.com/dualword/PebbleSDR-Qt License:GNU GPL*/
#ifndef AUDIO_H
#define AUDIO_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include <QObject>
#include <QStringList>
#include "cpx.h"

class Settings;
class Receiver;

class Audio : public QObject
{
    Q_OBJECT
public:
    Audio(QObject *parent = 0);
    ~Audio();
    virtual int StartInput(QString inputDeviceName, int inputSampleRate)=0;
    virtual int StartOutput(QString outputDeviceName, int outputSampleRate)=0;
	virtual int Stop()=0;
	virtual int Flush()=0;
	virtual int Pause()=0;
	virtual int Restart()=0;
    virtual void SendToOutput(CPX *_cpxBuf,int _numSamples, float gain = 1.0, bool mute = false)=0;
	virtual void ClearCounts()=0;

    //Creates either PortAudio or QTAudio device
    static Audio *Factory(Receiver *rcv, int framesPerBuffer, Settings *settings);

    //Returns a list of input devices for settings to chose from
    static QStringList InputDeviceList();
    static QStringList OutputDeviceList();

	static int DefaultOutputDevice();
	static int DefaultInputDevice();

	double inBufferUnderflowCount;
	double inBufferOverflowCount;
	double outBufferUnderflowCount;
	double outBufferOverflowCount;

	//Utility functions to convert samples
	float Int2Float(int sample) {return sample / 32767;}
	float UInt2Float(int sample) {return sample / 65535;}
	qint16 Float2Int(float sample) {return sample * 32767;}
	quint16 Float2UInt(float sample) {return sample * 65535;}

    static bool useQtAudio;

signals:

public slots:

protected:
	Settings *settings;
	Receiver *receiver;
	int inputSampleRate;
	int outputSampleRate;
	int framesPerBuffer; //#samples in each callback

};

#endif // AUDIO_H
