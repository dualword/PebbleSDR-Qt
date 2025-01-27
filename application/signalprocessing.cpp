//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include "string.h"

SignalProcessing::SignalProcessing(int sr, int ns)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPXBuf::malloc(numSamples);
}

SignalProcessing::~SignalProcessing(void)
{
	if (out != NULL)
		CPXBuf::free(out);
}

DelayLine::DelayLine(int s, int d) 
{
	size = s;
	delay = d;
	buffer = CPXBuf::malloc(size);
	CPXBuf::clear(buffer,size);
	head = 0;
	last = 0;
}
DelayLine::~DelayLine()
{
	if (buffer != NULL) CPXBuf::free(buffer);
}
void DelayLine::NewSample(CPX c)
{
	buffer[head] = c;
	last = head;
	//buffer order is newest to oldest, so head is decremented modulo size
	head = (head==0) ? size-1 : --head;
}
//Next sample in delay line, mod size of delay line to wrap (ring buffer)
CPX DelayLine::NextDelay(int i)
{
	//Last new value, plus delay, plus passed index, mod line size
	int next = (last + delay + i) % size;
	return buffer[next];
}
CPX DelayLine::operator [] (int i)
{
	return buffer[i];
}
//Convolution sum or Multiply And Accumulate (MAC)
//Key component for filter math
//In a FIR context, a "MAC" is the operation of multiplying a coefficient by the corresponding delayed data sample
//  and accumulating the result.
//DelayLine manages a circular buffer of samples with configurable delay
//coeff is the array of filter coefficients to apply
CPX DelayLine::MAC(double *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i];
		mac.im += buffer[next].im * coeff[i];
	}
    //This can generate NaN results if coeff aren't initialized properly, easy technique to catch while we debug
    if (mac.re != mac.re)
        mac.re = 0;
    if (mac.im != mac.im)
        mac.im = 0;

	mutex.unlock();
	return mac;
}
CPX DelayLine::MAC(CPX *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i].re;
		mac.im += buffer[next].im * coeff[i].im;
	}
    //This generates NaN results, easy technique to catch while we debug
    if (mac.re != mac.re)
        mac.re = 0;
    if (mac.im != mac.im)
        mac.im = 0;
    mutex.unlock();
	return mac;
}
#if (0)
void LMS(int numCoeff)
{
	int next;
	float sos = 0;
	//Compute the average squared value
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) & mask;
		sos += buffer[next].re;
	}
	float avgSOS = sos / numCoeff;

}
#endif
