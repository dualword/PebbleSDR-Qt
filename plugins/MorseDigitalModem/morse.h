#ifndef MORSE_H
#define MORSE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//#include "signalprocessing.h"
//#include "goertzel.h"
#include "qframe"
#include "QTextEdit"
#include "ui_data-morse.h"
#include "../../pebblelib/fldigifilters.h"
#include "morsecode.h"
#include "../../pebblelib/downconvert.h"
//#include "demod.h"
#include <QMutex>
#include "../../pebblelib/digital_modem_interfaces.h"

class Receiver;

//Utility functions from fldigi
inline double clamp(double x, double min, double max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}

//Weight is the factor to weight the input when calculating the average
//Weight=20: (input * 1/20) + average * (19/20) - Short average, only 20 loops to make average == steady input
//Weight=800: (input * 1/800) + average * (799/800) - Long average, stead input would take 800 to == average
inline double decayavg(double average, double input, double weight)
{
    if (weight <= 1.0) return input;
    return input * (1.0 / weight) + average * (1.0 - (1.0 / weight));
}

//We need to inherit from QObject so we can filter events.
//Remove if we eventually create a separate 'morse widget' object for UI
class Morse : public QObject, public DigitalModemInterface
{
    Q_OBJECT
    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)


public:
    Morse();
    ~Morse();
    //DigitalModemInterface
    void SetSampleRate(int _sampleRate, int _sampleCount);
    void SetDemodMode(DEMODMODE _demodMode);
    CPX * ProcessBlock(CPX * in);
    void SetupDataUi(QWidget *parent);
    QString GetPluginName();
    QString GetDescription();
    QObject *asQObject() {return (QObject *)this;}


    //Returns tcw in ms for any given WPM
    int WpmToTcw(int w);
    //Returns wpm for any given tcw
    int TcwToWpm(int t);

    void OutputData(const char *d);
    void OutputData(const char c);

    quint32 WPMToUsec(int w);
    int UsecToWPM(quint32 u);

public slots:
    void squelchChanged(int v);
    void refreshOutput();
    void resetOutput();
    void onBoxChecked(bool b);
    void outputOptionChanged(int s);
    void lockWPMChanged(bool b);

signals:
    void newOutput();
    void Testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
    void Testbench(int _length, double* _buf, double _sampleRate, int _profile);
    bool AddProfile(QString profileName, int profileNumber); //false if profilenumber already exists
    void RemoveProfile(quint16 profileNumber);


protected:
    //From SignalProcessing
    int numSamples;
    int sampleRate;
    CPX *out;

    Ui::dataMorse *dataUi;
    CPXBuf *workingBuf;
    int demodMode; //See DEMODMODE for enum, move to lib?
    bool useNormalizingThreshold; //Switch how we compare tone values to determine rising or falling

    int modemFrequency; //CW tone we're looking for.  This is the goertzel freq or the NCO freq for mixer approach
    int modemSampleRate; //Modem (Goretzel or other) only needs 8k sample rate.  Bigger means more bins and slower
    int modemBandwidth; //Desired bw after decimation

    //Characters are output from the main receive thread and can't be output directly to textedit
    //due to Qt requirement that only main thread does Gui output
    //So we put characters into this buffer in thread and pick them up in GUI with timer
    char outputBuf[256]; //Way bigger than we need
    int outputBufIndex; //Position in output buf of next char, zero means buffer is empty
    QMutex outputBufMutex;
    bool outputOn;

    CDownConvert modemDownConvert; //Get to modem rate and mix

    CPX mixer(CPX in);
    void syncFilterWithWpm();

    MorseCode morseCode;
    void decode_stream(double value);
    double agc_peak; // threshold for tone detection

    void init();

    //Fldigi samplerate is 8x
    //Filter does MAC every DEC_RATIO samples

    const static int DEC_RATIO	= 16; //Decimation Ratio replaced with modemDecimateFactor
    const static int CW_FIRLEN = 512;

    //Filters
    const static int TRACKING_FILTER_SIZE = 16;
    C_FIR_filter	*hilbert;   // Hilbert filter precedes sinc filter
    //!fftfilt			*cw_FFT_filter; // sinc / matched filter
    C_FIR_filter	*cw_FIR_filter; // linear phase finite impulse response filter

    Cmovavg		*bitfilter;

    //Received CW speed can be fixed (set by user) or track actual dot/dash lengths being received
    Cmovavg *trackingfilter;
    void updateAdaptiveThreshold(quint32 idotUsec, quint32 idashUsec);

    const int trackingWPMRange = 10; //Tracking range for CWTRACK (WPM)
    const int lowerWPMLimit = 5; //Lower RX limit (WPM)
    const int upperWPMLimit = 50; // Upper RX limit (WPM)


    int bitfilterlen;

    const bool useFFTFilter = false;

    int FilterFFTLen;

    //Use by NCO in mixer to mix cwToneFrequency
    double		phaseacc;
    double		FFTphase;
    double		FIRphase;

    double		FFTvalue;
    double		FIRvalue;


    // Receive buffering
    const static int RECEIVE_CAPACITY = 256;
    //This holds dots and dashes as they are received, way longer than any letter or phrase
    char dotDashBuf[RECEIVE_CAPACITY];

    // dotDash buffer current location
    int dotDashBufIndex;

    void resetDotDashBuf();
    void resetModemClock();

    //Tone timing
    //Modem clock ticks once for every sample, each tick represents 1/modemSampleRate second
    //Times tones and silence
    const static int USECS_PER_SEC = 1000000;	// Microseconds in a second
    //All usec variable are quint32 because modemClock is quint32 and we don't want conversion errors
    //quint32 is approx 71 minutes (70*60*USEC_PER_SEC)
    quint32 modemClock; //!!Do we have to do anything special to handle overflow wrap back to 0?
    quint32 modemClockToUsec(unsigned int earlier, unsigned int later);

    quint32 toneStart;		// Tone start timestamp
    quint32 toneEnd;		// Tone end timestamp
    quint32 usecNoiseSpike;		// Initially ignore any tone < 10mS
    quint32 usecLastMark = 0;	// length of last dot
    quint32 usecLastSpace = 0;	// length of last dot
    quint32 usecMark = 0;		// Time difference in usecs
    quint32 usecSpace = 0;
    quint32 usecElementThreshold = 0; //Space between dot/dash in char
    void calcDotDashLength(int _speed, quint32 & _usecDot, quint32 & _usecDash);

    void syncTiming();


    enum CW_EVENT {TONE_EVENT, NO_TONE_EVENT};
    enum DECODE_STATE {IDLE, MARK_TIMING, INTER_ELEMENT_TIMING, WORD_SPACE_TIMING};
    bool stateMachine(CW_EVENT event);
    DECODE_STATE		receiveState;	// Indicates receive state
    DECODE_STATE		lastReceiveState;
    CW_EVENT		cw_event;			// functions used by cw process routine

    // user configurable data - local copy passed in from gui
    double	squelchMetric;
    double squelchIncrement; //Slider increments
    double squelchValue; //If squelch is on, this is compared to metric
    bool squelchEnabled;

    //Fixed speed or computed speed based on actual dot/dash timing
    int wpmSpeedCurrent;				// Initially 18 WPM
    int wpmSpeedFilter;     //Speed filter is initialized for
    quint32 usecDotCurrent;		// Length of a receive Dot, in Usec based on receiveSpeed
    quint32 usecDashCurrent;		// Length of a receive Dash, in Usec based on receiveSpeed
    //Used to restore to base case when something changes or we get lost
    const int wpmSpeedInit = 18;
    quint32 usecDotInit; //Was cw_send_dot_length
    quint32 usecDashInit;

    const bool useLowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB


    //Fastest we can handle at sample rate
    quint32 usecPerSample;
    quint32 usecLongestMark;
    quint32 usecShortestMark;
    // Receiving parameters:

    quint32 usecAdaptiveThreshold;		// 2-dot threshold for adaptive speed

    const char *spaceTiming(bool lookingForChar);
    void outputString(const char *outStr);
    void addMarkToDotDash();
    bool markHandled;

    enum OUTPUT_MODE{CHAR_ONLY,CHAR_AND_DOTDASH,DOTDASH};
    OUTPUT_MODE outputMode; //What we display in output

    //Replaces tracking variables
    bool lockWPM; //Locks the current WPM and disables tracking

    QString stateToString(DECODE_STATE state);
    void dumpStateMachine(QString why);
};

#endif // MORSE_H
