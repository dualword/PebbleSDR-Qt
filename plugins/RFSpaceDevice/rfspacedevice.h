#ifndef RFSPACEDEVICE_H
#define RFSPACEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "producerconsumer.h"
#include "usbutil.h"
#include "cpx.h"
#include "ad6620.h"
#include "ui_sdriqoptions.h"

class RFSpaceDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	//Data blocks are fixed at 8192 bytes by SDR-IQ
	//4096 samples at 2 bytes (short) per sample
	static const quint16 samplesPerDataBlock = 4096; //shorts
	static const quint16 dataBlockSize = samplesPerDataBlock * 2;

	RFSpaceDevice();
	~RFSpaceDevice();

	//Required
	bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();
	void ReadSettings();
	void WriteSettings();
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);
	//Display device option widget in settings dialog
	void SetupOptionUi(QWidget *parent);

private slots:
	void rfGainChanged(int i);
	void ifGainChanged(int i);

private:
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	ProducerConsumer producerConsumer;
	USBUtil *usbUtil;
	AD6620 *ad6620;

	//Capture N blocks of data
	unsigned CaptureBlocks(unsigned numBlocks);
	unsigned StartCapture();
	unsigned StopCapture();
	bool ReadDataBlock(CPX cpxBuf[], unsigned cpxBufSize);
	void FlushDataBlocks();
	//Write 5 bytes of data
	bool SetRegisters(unsigned adrs, qint32 data);
	bool SetAD6620Raw(unsigned adrs, short d0, short d1, short d2, short d3, short d4);
	bool SetRFGain(qint8 g);
	unsigned GetRFGain();
	bool SetIFGain(qint8 g);
	//SDR-IQ General Control Items
	bool RequestTargetName();
	bool RequestTargetSerialNumber();
	bool RequestInterfaceVersion(); //Version * 100, ie 1.23 returned as 123
	bool RequestFirmwareVersion();
	bool RequestBootcodeVersion();
	unsigned StatusCode();
	QString StatusString(unsigned char code);


	Ui::SdrIqOptions *optionUi;

	QMutex mutex;
	CPX *inBuffer;
	int inBufferSize;
	unsigned char *readBuf;
	unsigned char *readBufProducer; //Exclusively for producer thread to avoid possible buffer contention with main thread

	int msTimeOut;


	//Returned data values from SDR-IQ
	QString targetName;
	QString serialNumber;
	unsigned interfaceVersion;
	unsigned firmwareVersion;
	unsigned bootcodeVersion;
	unsigned statusCode;
	QString statusString;
	unsigned lastReceiverCommand;
	double receiverFrequency;
	int rfGain;
	int ifGain;
	AD6620::BANDWIDTH bandwidth;

	//Remove
	//Settings
	double sDefaultStartup;
	double sLow;
	double sHigh;
	int sStartupMode;
	double sGain;
	int sBandwidth;
	int sRFGain;
	int sIFGain;

	double SetFrequency(double freq);
	double GetFrequency();
};


#endif // EXAMPLESDRDEVICE_H
