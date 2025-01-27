#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"

class AGC :
	public SignalProcessing
{
public:
    //Keep in sync with order of selection box in receiverWidget
    enum AGCMODE {FAST, MED, SLOW, LONG, OFF};

	AGC(int sampleRate, int nSamples);
	~AGC(void);
	void setAgcMode(AGCMODE m);
    void setAgcThreshold(int g);
    int getAgcThreshold();

	CPX * ProcessBlock(CPX *in);
    //CPX * ProcessBlock2(CPX *in);

    //CPX *ProcessBlock3(CPX *in); //Testing cuteSDR algorithm

    void SetParameters(bool UseHang, int Threshold, int ManualGain, int SlopeFactor, int Decay);
private:
	AGCMODE agcMode;
	DelayLine *agcDelay;

	//These values are used to configure agc for different modes, they are not used directly
	float modeHangTimeMs;
	float modeFastHangTimeMs;
	float modeDecayTimeMs;
	float modeAttackTimeMs;
	float modeHangWeight;

	float gainNow;
	float gainPrev;
	float gainLimit;
	//Gain limits
	float gainFix;
	float gainTop;
	float gainBottom;

	float hangThreshold;
	//Times are all normalized to number of samples
	int hangSamples;
	int fastHangSamples;
	int hangCount;

	int attackSamples;
	float attackWeightNew;
	float attackWeightLast;
	float fastAttackWeightNew;
	float fastAttackWeightLast;

	int decaySamples;
	float decayWeightNew;
	float decayWeightLast;
	float fastDecayWeightNew;
	float fastDecayWeightLast;

	int delayTime;
	int fastDelayTime;

    //CuteSDR implementation
    int thresholdFromUi;

#define MAX_DELAY_BUF 2048

    //internal copy of AGC settings parameters
    bool m_UseHang;
    int m_Threshold;
    int m_ManualGain;
    int m_Slope;
    int m_Decay;
    double m_SampleRate;

    double m_SlopeFactor;
    double m_ManualAgcGain;

    double m_DecayAve;
    double m_AttackAve;

    double m_AttackRiseAlpha;
    double m_AttackFallAlpha;
    double m_DecayRiseAlpha;
    double m_DecayFallAlpha;

    double m_FixedGain;
    double m_Knee;
    double m_GainSlope;
    double m_Peak;

    int m_SigDelayPtr;
    int m_MagBufPos;
    int m_DelaySize;
    int m_DelaySamples;
    int m_WindowSamples;
    int m_HangTime;
    int m_HangTimer;

    QMutex m_Mutex;		//for keeping threads from stomping on each other
    CPX m_SigDelayBuf[MAX_DELAY_BUF];
    double m_MagBuf[MAX_DELAY_BUF];





	//Utility function, used in many calculations exp(x) = e^x
	inline float eVal(float x) {return exp(-1000.0 / (x * sampleRate));}
};
