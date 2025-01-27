//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "noisefilter.h"

NoiseFilter::NoiseFilter(int sr, int ns):
	SignalProcessing(sr,ns)
{

	anfDelaySize = 512; //dttsp 512, SDRMax 1024
    anfDelaySamples = 64;
	anfAdaptiveFilterSize = 45; //dttsp 45, SDRMax 256
	anfAdaptationRate = 0.01; //dttsp 0.01, SDRMax 0.005
	anfLeakage = 0.00001; //dttsp 0.00001, SDRMax 0.01
    anfDelay = new DelayLine(anfDelaySize, anfDelaySamples); //64 sample delay line
    anfCoeff = new CPX[anfAdaptiveFilterSize];
	anfEnabled = false;
}

NoiseFilter::~NoiseFilter(void)
{
}
void NoiseFilter::setAnfEnabled(bool b)
{
	if (b)
		//Clean out any garbage from prior filtering
        memset(anfCoeff,0.0,sizeof(anfCoeff) * anfAdaptiveFilterSize);

	anfEnabled = b;
}

//Called from main recieve chain if enabled
/*
Algorithm references
	dttsp lmadf.c

*/
CPX * NoiseFilter::ProcessBlock(CPX *in)
{
	int size = numSamples;
	if (!anfEnabled)
	{
		return in;
	}

    CPX sos; //Sum of squares
    CPX accum;
	
    CPX error;

    double scl1 = 0;
    CPX scl2;

    //double factor2 = 0;
	CPX nxtDelay;
	//How rapidly do we adjust for changing signals, bands, etc
	scl1 = 1.0 - anfAdaptationRate * anfLeakage;

	for (int i = 0; i < size; i++)
	{
		//Add the current sample to the delay line
		anfDelay->NewSample(in[i]);

        accum.clear();
        sos.clear();
		//We don't have to run through entire delay line, just enough to calc noise data
		//For each sample, accumulate 256 (adapt size) samples, delayed by 64 samples
		//This is the basic filter step and the coefficients tell us how to weigh historical samples
		//or,if preset, filter for desired frequencies.
		for (int j = 0; j < anfAdaptiveFilterSize; j++)
		{
			nxtDelay = anfDelay->NextDelay(j);

            sos.re += (nxtDelay.re * nxtDelay.re);
            sos.im += (nxtDelay.im * nxtDelay.im);
            //dttsp doesn't accumulate sums, bug in dttsp?
            accum.re += anfCoeff[j].re * nxtDelay.re;
            accum.im += anfCoeff[j].im * nxtDelay.im;

		}
        //This does the same as accum above, but requires an extra loop
        //Maybe we should update MAC to also return sum or squares result?
        out[i].re = accum.re * 1.25; //Bit of gain to compensate for filter
        out[i].im = accum.im * 1.25;

		//This is the delta between the actual current sample, and MAC from delay line (reference signal)
		//or if we're comparing to a desired CW sine wave, the diff from that
		//It drives the Least Means Square (LMS) algorithm
		//errorSignal = (in[i] - MAC * adaptionRate) / SOS
        //CPX error = in[i] - anfDelay->MAC(anfCoeff,anfAdaptiveFilterSize);
        error = in[i] - accum;

        scl2.re = (anfAdaptationRate / (sos.re + 1e-10)); //avoid divide by zero trick
        error.re *= scl2.re;

        scl2.im = (anfAdaptationRate / (sos.im + 1e-10));
        error.im *= scl2.im;

		//And calculate tne new coefficients for next sample
		/*
		See Doug Smith, pg 8-2 for Adaptive filters, Least Mean Squared, Leakage, etc
		h[t+1] = h[t] + 2 * adaptationRate * errorSignal * sample
		*/

		for (int j = 0; j < anfAdaptiveFilterSize; j++)
		{
			nxtDelay = anfDelay->NextDelay(j);
			//Weighted average based on anfLeakage
			//AdaptationRate is between 0 and 1 and determines how fast we stabilize filter
			//anfCoeff[j] = anfCoeff[j-1] * anfLeakage + (1.0 - anfLeakage) * anfAdaptationRate * nxtDelay.re * errorSignal;
            anfCoeff[j].re = anfCoeff[j].re * scl1 + error.re * nxtDelay.re;
            anfCoeff[j].im = anfCoeff[j].im * scl1 + error.im * nxtDelay.im;
        }
	}
	return out;
}
