//////////////////////////////////////////////////////////////////////
// sdrinterface.h: interface for the CSdrInterface class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-16  Added Frequency range logic for optional down converter modules
//	2011-08-07  Added WFM Support
/////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//=============================================================================
#ifndef SDRINTERFACE_H
#define SDRINTERFACE_H

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include "netiobase.h"
#include "ad6620.h"
#include "protocoldefs.h"
#include "dataprocess.h"
#include "cpx.h"

class SDR_IP;

/////////////////////////////////////////////////////////////////////
// Derived class from NetIOBase for all the custom SDR msg processing
/////////////////////////////////////////////////////////////////////
class CSdrInterface : public CNetio
{
    Q_OBJECT
public:
    CSdrInterface(SDR_IP *_sdr_ip);
    ~CSdrInterface();
    enum eRadioType {
        SDR14,
        SDRIQ,
        SDRIP,
        NETSDR,
        CLOUDSDR
    };
    // virtual functioncalled by TCP thread with new msg from radio to parse
    void ParseAscpMsg(CAscpRxMsg *pMsg);
    void SetRx2Parameters(double Rx2Gain, double Rx2Phase);

    void GetSdrInfo();
    void SetChannelMode(qint32 channelmode);
    void ReqStatus();
    void StartSdr();
    void StopSdr();
    void StopIO();	//stops IO threads

    void SetSdrRfGain(qint32 gain);
    qint32 GetSdrRfGain(){return m_RfGain;}
    quint64 SetRxFreq(quint64 freq);
    void KeepAlive();
    void SetSdrBandwidthIndex(qint32 bwindex);
    void SetSdrBandwidth(qint32 bw);
    quint32 GetSdrMaxBandwidth(){return m_SampleRate;}

    void SetRadioType(qint32 radiotype ){m_RadioType = (eRadioType)radiotype;}
    qint32 GetRadioType(){return (qint32)m_RadioType;}

    double GetSdrSampleRate(){return m_SampleRate;}
    quint32 LookUpBandwidth(qint32 sr);

    //virtual function called by UDP thread with new raw data to process
    void ProcessUdpData(char* pBuf, qint64 length);

    //bunch of public members containing sdr related information and data
    QString m_DeviceName;
    QString m_SerialNum;
    float m_BootRev;
    float m_AppRev;
    float m_HardwareRev;
    int m_MissedPackets;

signals:
    void NewInfoData();			//emitted when sdr information is received after GetSdrInfo()
    void FreqChange(int freq);	//emitted if requested frequency has been clamped by radio

private:
    SDR_IP *sdr_ip;

    void SendAck(quint8 chan);
    //Utility function, were public
    qint32 GetMaxBWFromIndex(qint32 index);
    double GetSampleRateFromIndex(qint32 index);


    eRadioType m_RadioType;
    bool m_Running;
    qint32 m_KeepAliveCounter;
    double m_SampleRate;
    quint64 m_BaseFrequencyRangeMin;
    quint64 m_BaseFrequencyRangeMax;
    quint64 m_OptionFrequencyRangeMin;
    quint64 m_OptionFrequencyRangeMax;
    Cad6620 m_AD6620;
    qint32 m_ChannelMode;
    quint8 m_Channel;
    qint32 m_RfGain;
    double m_GainCalibrationOffset;
    qint32 m_BandwidthIndex; //Current BW from table
    quint64 m_CurrentFrequency;
    qint32 m_MaxBandwidth;
    bool m_InvertSpectrum;

    //For decodePacket()
    int m_PacketSize;
    quint16 m_LastSeqNum;
    CPX* m_pInQueue;
    quint32 numSamplesInBuffer; //Number of samples in working producer buffer


};

#if 0



	//NCO spur management commands for ManageNCOSpurOffsets(...)
	enum eNCOSPURCMD {
		NCOSPUR_CMD_SET,
		NCOSPUR_CMD_STARTCAL,
		NCOSPUR_CMD_READ
	};



	bool GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
									double MaxdB, double MindB,
									qint32 StartFreq, qint32 StopFreq,
									qint32* OutBuf );
	void ScreenUpdateDone(){m_ScreenUpateFinished = true;}
	void ManageNCOSpurOffsets( eNCOSPURCMD cmd, double* pNCONullValueI,  double* pNCONullValueQ);



	//GUI Public settings and access functions

	void SetSoundCardSelection(qint32 SoundInIndex, qint32 SoundOutIndex,bool StereoOut)
				{ m_pSoundCardOut->Stop(); m_SoundInIndex = SoundInIndex;
					m_SoundOutIndex = SoundOutIndex;  m_StereoOut = StereoOut;}

	int GetRateError(){return m_pSoundCardOut->GetRateError();}
	void SetVolume(qint32 vol){ m_pSoundCardOut->SetVolume(vol); }


	qint32 GetSdrRfGain(){return m_RfGain;}

	double GetSdrSampleRate(){return m_SampleRate;}
	quint32 GetSdrMaxBandwidth(){return m_SampleRate;}

	void SetFftSize(qint32 size);
	quint32 GetSdrFftSize(){return m_FftSize;}

	void SetFftAve(qint32 ave);
	quint32 GetSdrFftAve(){return m_FftAve;}

	void SetMaxDisplayRate(int updatespersec){m_MaxDisplayRate = updatespersec;
							m_DisplaySkipValue = m_SampleRate/(m_FftSize*m_MaxDisplayRate);
							m_DisplaySkipCounter = 0; }

	void SetDemod(int Mode, tDemodInfo CurrentDemodInfo);
	void SetDemodFreq(qint64 Freq){m_Demodulator.SetDemodFreq((TYPEREAL)Freq);}

	void SetupNoiseProc(tNoiseProcdInfo* pNoiseProcSettings);

	double GetSMeterPeak(){return m_Demodulator.GetSMeterPeak() + m_GainCalibrationOffset - m_RfGain;}
	double GetSMeterAve(){return m_Demodulator.GetSMeterAve() + m_GainCalibrationOffset - m_RfGain;}

	void SetSpectrumInversion(bool Invert){m_InvertSpectrum = Invert;}
	void SetUSFmVersion(bool USFm){m_USFm = USFm;}
	bool GetUSFmVersion(){return m_USFm;}

	//access to WFM mode status/data
	int GetStereoLock(int* pPilotLock){ return m_Demodulator.GetStereoLock(pPilotLock);}
	int GetNextRdsGroupData(tRDS_GROUPS* pGroupData){return m_Demodulator.GetNextRdsGroupData(pGroupData);}

signals:
	void NewFftData();			//emitted when new FFT data is available

private:
	void Start6620Download();
	void NcoSpurCalibrate(TYPECPX* pData, qint32 NumSamples);


	bool m_ScreenUpateFinished;
	bool m_StereoOut;
	bool m_USFm;
	qint32 m_DisplaySkipCounter;
	qint32 m_DisplaySkipValue;
	qint32 m_FftSize;
	qint32 m_FftAve;
	qint32 m_FftBufPos;
	qint32 m_MaxDisplayRate;
	qint32 m_SoundInIndex;
	qint32 m_SoundOutIndex;
	TYPECPX m_DataBuf[MAX_FFT_SIZE];


	bool m_NcoSpurCalActive;	//NCO spur reduction variables
	qint32 m_NcoSpurCalCount;
	double m_NCOSpurOffsetI;
	double m_NCOSpurOffsetQ;

	CFft m_Fft;
	CDemodulator m_Demodulator;
	CNoiseProc m_NoiseProc;
	CSoundOut* m_pSoundCardOut;

CIir m_Iir;

};


#endif
#endif // SDRINTERFACE_H
