#ifndef MORSEGENDEVICE_H
#define MORSEGENDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_morsegendevice.h"
#include "morsegen.h"

class MorseGenDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	MorseGenDevice();
	~MorseGenDevice();

	//Required
	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool command(StandardCommands _cmd, QVariant _arg);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

private slots:
	void resetButtonClicked(bool clicked);
private:
	void readSettings();
	void writeSettings();
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	void setupOptionUi(QWidget *parent);

	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPX *consumerBuffer;
	quint16 producerIndex;
	CPX *producerFreeBufPtr; //Treat as array of CPX

	Ui::MorseGenOptions *optionUi;

	QElapsedTimer elapsedTimer;
	qint64 nsPerBuffer; //How fast do we have to output a buffer of data to match recorded sample rate

	static const quint32 c_dbFadeRange = 20; //For random fade generation

	CPX *m_outBuf;

	CPX *m_outBuf1;
	CPX *m_outBuf2;
	CPX *m_outBuf3;
	CPX *m_outBuf4;
	CPX *m_outBuf5;

	MorseGen *m_morseGen1;
	MorseGen *m_morseGen2;
	MorseGen *m_morseGen3;
	MorseGen *m_morseGen4;
	MorseGen *m_morseGen5;

	QString m_sampleText[5];
	CPX nextNoiseSample(double _dbGain);


	void generate();
};
#endif // MORSEGENDEVICE_H
