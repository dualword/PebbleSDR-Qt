//////////////////////////////////////////////////////////////////////
// fractresampler.h: interface for the CFractResampler class.
//
//  This class implements a fractional resampler that can be used to
//convert between different sample rates
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef FRACTRESAMPLER_H
#define FRACTRESAMPLER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "fir.h"
#include "iir.h"
#include "downconvert.h"
#include "../demod/rbdsconstants.h"

class CFractResampler  
{
public:
	CFractResampler();
	virtual ~CFractResampler();

	void Init(int MaxInputSize);
	//overloaded functions for processing different data types
	int Resample( int InLength, TYPEREAL Rate, TYPEREAL* pInBuf, TYPEREAL* pOutBuf);
	int Resample( int InLength, TYPEREAL Rate, TYPECPX* pInBuf, TYPECPX* pOutBuf);
	int Resample( int InLength, TYPEREAL Rate, TYPEREAL* pInBuf, TYPEMONO16* pOutBuf, TYPEREAL gain);
	int Resample( int InLength, TYPEREAL Rate, TYPECPX* pInBuf, TYPESTEREO16* pOutBuf, TYPEREAL gain);

private:
	TYPEREAL m_FloatTime;	//floating pt output time accumulator
	TYPEREAL* m_pSinc;	//ptr to sinc table
	TYPECPX* m_pInputBuf;	//internal working input sample buffer
};

#endif // FRACTRESAMPLER_H
