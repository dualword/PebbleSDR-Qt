//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "producerconsumer.h"

/*
 * This uses the QT producer/consumer model to avoid more complex and deadlock prone mutex's
 * QSemaphores protects the producer thread's buffer pointer  and the consumer threads' buffer pointer in a circular buffer
 *
 * 2/12/14: New model
 * Producer ReleaseFilledBuffer emits signal which is connected to Consumer new data slot
 * Consumer should process as many filled buffers as possible before returning
 *
 * Producer and Consumer threads now has a timing interval which is calculated based on sample rate and samples/buffer
 * This also reduces CPU load because we sleep for a safe amount of time between polling for new data
 *
 * 2/26/14: Significant update to use QTimers in threads.  QThread::msSleep() actually blocks, but QTimer() let other threads run
 */

ProducerConsumer::ProducerConsumer()
{
	//Non blocking gives us more control and as long as polling interval is set correctly, same CPU as blocking
	useBlockingAcquire = false;

    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;

    producerWorker = NULL;
    producerWorkerThread = NULL;
    consumerWorker = NULL;
    consumerWorkerThread = NULL;

	//0 interval is reserved and sets timer that is triggered everytime event loop is empty
	msProducerInterval = 1;
	msConsumerInterval = 1;
}

//This can get called on an existing ProducerConsumer object
//Make sure we reset everything
void ProducerConsumer::Initialize(cbProducerConsumer _producerWorker, cbProducerConsumer _consumerWorker, int _numDataBufs, int _producerBufferSize)
{
    //Defensive
    if (_producerBufferSize == 0)
        return; //Can't create empty buffer

    cbProducerWorker = _producerWorker;
    cbConsumerWorker = _consumerWorker;

    numDataBufs = _numDataBufs;

    //2 bytes per sample, framesPerBuffer samples after decimate
    producerBufferSize = _producerBufferSize;
    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            delete (producerBuffer[i]);
        delete (producerBuffer);
    }
    producerBuffer = new unsigned char *[numDataBufs];
    for (int i=0; i<numDataBufs; i++)
        producerBuffer[i] = new unsigned char [producerBufferSize];

    //Start out with all producer buffers available
    if (semNumFreeBuffers != NULL)
        delete semNumFreeBuffers;
    semNumFreeBuffers = new QSemaphore(numDataBufs);

    if (semNumFilledBuffers != NULL)
        delete semNumFilledBuffers;
    //Init with zero available
    semNumFilledBuffers = new QSemaphore(0);

    nextProducerDataBuf = nextConsumerDataBuf = 0;
    /*
        Thread                      Worker
        producerWorkerThread->start();
        Signal started() ---------->Slot Start()        Thread tells works to start

                                    Stop()              Sets bool stop=true to exit forever loop

        Slot quit() <-------------- Signal finished()   Worker tells thread it's finished after detecting interupt request
        Slot finished() ----------->Slot Stop()         Thread tells worker everything is done

    */


    //New worker pattern that replaces subclassed QThread.  Recommended new QT 5 pattern
    if (producerWorkerThread == NULL) {
		producerWorkerThread = new QThread(this);
        producerWorkerThread->setObjectName("PebbleProducer");
    }
    if (producerWorker == NULL) {
		producerWorker = new ProducerWorker(cbProducerWorker);
		connect(producerWorkerThread,&QThread::started,producerWorker,&ProducerWorker::start);
		connect(producerWorkerThread, &QThread::finished, producerWorker,&ProducerWorker::stop);
		connect(this,&ProducerConsumer::CheckNewData, producerWorker, &ProducerWorker::checkNewData);
	}
    if (consumerWorkerThread == NULL) {
		consumerWorkerThread = new QThread(this);
        consumerWorkerThread->setObjectName("PebbleConsumer");
    }
    if (consumerWorker == NULL) {
		consumerWorker = new ConsumerWorker(cbConsumerWorker);
		connect(consumerWorkerThread,&QThread::started,consumerWorker,&ConsumerWorker::start);
		connect(consumerWorkerThread, &QThread::finished, consumerWorker,&ConsumerWorker::stop);
		connect(this,&ProducerConsumer::ProcessNewData, consumerWorker, &ConsumerWorker::processNewData);
    }

}

//Make sure sampleRate and samplesPerBuffer are in same units
//Bytes, CPX's, etc
void ProducerConsumer::SetConsumerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer)
{
	//1 sample every N ms X number of samples per buffer = how long it takes device to fill a buffer
	quint16 msToFillBuffer = (1000.0 / _sampleRate) * _samplesPerBuffer;
	//Set safe interval (experiment here)
	//Use something that results in a non-cyclic interval to avoid checking constantly at the wrong time
	quint16 msBufferInterval = msToFillBuffer * 0.66;
	//Interval can never be 0 which is a special case for QTimer
	msConsumerInterval = msBufferInterval > 0 ? msBufferInterval : 1;
	qDebug()<<"Consumer checks every "<<msConsumerInterval<<" ms "<<"SampleRate | SamplesPerBuffer"<<_sampleRate<<_samplesPerBuffer;
	if (consumerWorker != NULL)
		consumerWorker->SetPollingInterval(msConsumerInterval);

}

//Only if producer is polling for data, not needed if producer is triggered via signal
//Call after initialize
void ProducerConsumer::SetProducerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer)
{
	//1 sample every N ms X number of samples per buffer = how long it takes device to fill a buffer
	quint16 msToFillBuffer = (1000.0 / _sampleRate) * _samplesPerBuffer;
	//Set safe interval (experiment here)
	quint16 msBufferInterval = msToFillBuffer * 0.66;
	//Interval can never be 0 which is a special case for QTimer
	msProducerInterval = msBufferInterval > 0 ? msBufferInterval : 1;
	qDebug()<<"Producer checks every "<<msProducerInterval<<" ms"<<"SampleRate | SamplesPerBuffer"<<_sampleRate<<_samplesPerBuffer;;
	if (producerWorker != NULL)
		producerWorker->SetPollingInterval(msProducerInterval);
}


bool ProducerConsumer::Start(bool _producer, bool _consumer)
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    producerThreadIsRunning = false;
    consumerThreadIsRunning = false;
    //Note: QT only allows us to set thread priority on a running thread or in start()
    if (_consumer) {
		//Don't move worker to thread unless thread is started.  Signals can still be connected to consumerWorker
		consumerWorker->moveToThread(consumerWorkerThread);
        consumerWorkerThread->start();
		//Thread priorities from CuteSdr model
		//Producer get highest, time critical, slices in order to be able to keep up with data
		//Consumer gets next highest, in order to run faster than main UI thread
		consumerWorkerThread->setPriority(QThread::HighestPriority);
        consumerThreadIsRunning = true;
    }
	if (_producer) {
		producerWorker->moveToThread(producerWorkerThread);
		producerWorkerThread->start();
		producerWorkerThread->setPriority(QThread::NormalPriority);
        producerThreadIsRunning = true;
    }
    return true;
}

bool ProducerConsumer::Stop()
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    if (producerThreadIsRunning) {
		producerWorkerThread->quit();
        producerThreadIsRunning = false;
    }

    if (consumerThreadIsRunning) {
		consumerWorkerThread->quit();
        consumerThreadIsRunning = false;
    }
	return true;
}

//Not generally needed because we reset everything in Initialize, but here in case we need to restart due to overflow
bool ProducerConsumer::Reset()
{
	nextProducerDataBuf = nextConsumerDataBuf = 0;
	//Reset filled buffers to zero
	semNumFilledBuffers->release(semNumFilledBuffers->available());
	qDebug()<<"ProducerConsumer reset - numFilled = "<<semNumFilledBuffers->available();

	//Reset free buffers to max
	semNumFreeBuffers->acquire(numDataBufs - semNumFreeBuffers->available());
	qDebug()<<"ProducerConsumer reset - numFree = "<<semNumFreeBuffers->available();

	return true;
}

bool ProducerConsumer::IsFreeBufferAvailable()
{
    if (semNumFreeBuffers == NULL)
        return false; //Not initialized yet

    //Make sure we have at least 1 data buffer available without blocking
    int freeBuf = semNumFreeBuffers->available();
    if (freeBuf == 0) {
        //qDebug()<<"No free buffer available, ignoring block.";
        return false;
    }
    return true;
}

//This call is the only way we can get a producer buffer ptr
//We can use the ptr until ReleaeFreeBuffer is called
//Returns NULL if no free buffer can be acquired
unsigned char* ProducerConsumer::AcquireFreeBuffer(quint16 _timeout)
{
    if (semNumFreeBuffers == NULL)
        return NULL; //Not initialized yet
    if (!useBlockingAcquire) {
        //We can't block with just acquire() because thread will never finish
        //If we can't get a buffer in N ms, return NULL and caller will exit
        if (semNumFreeBuffers->tryAcquire(1, _timeout)) {
            return producerBuffer[nextProducerDataBuf];
        } else {
            return NULL;
        }
    } else {
        //Blocks, but yields
        semNumFreeBuffers->acquire();
        return producerBuffer[nextProducerDataBuf];
    }
}

void ProducerConsumer::ReleaseFreeBuffer()
{
    nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;
    semNumFreeBuffers->release();
	emit CheckNewData();
}

void ProducerConsumer::PutbackFreeBuffer()
{
    semNumFreeBuffers->release();
}

unsigned char *ProducerConsumer::AcquireFilledBuffer(quint16 _timeout)
{
    if (semNumFilledBuffers == NULL)
        return NULL; //Not initialized yet

	if (!useBlockingAcquire) {
        //We can't block with just acquire() because thread will never finish
        //If we can't get a buffer in N ms, return NULL and caller will exit
        if (semNumFilledBuffers->tryAcquire(1, _timeout)) {
            return producerBuffer[nextConsumerDataBuf];
        } else {
            return NULL;
        }
    } else {
		//This will block the thread waiting for a filled buffer
		//Internally QSemaphore wraps QWaitCondition, see QWaitCondition for lower level example
		//Does QWaitCondition wait() just block, or yield
        semNumFilledBuffers->acquire();
        return producerBuffer[nextConsumerDataBuf];
    }
}

//We have to consider the case where the producer is being called more frequently than the consumer
//This can be the case if we are running without a producer thread and filling the producer buffers
//directly.  For example in a QTcpSocket readyRead() signal handler.
void ProducerConsumer::ReleaseFilledBuffer()
{
    nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs; //Increment producer pointer
    semNumFilledBuffers->release();
	//Consumer worker is called via thread loop, also via signal
	//Don't really need both, but experiementing with what's more efficient
	emit ProcessNewData();
}

void ProducerConsumer::PutbackFilledBuffer()
{
	semNumFilledBuffers->release();
}

quint16 ProducerConsumer::GetPercentageFree()
{
	if (semNumFreeBuffers != NULL)
		return (semNumFreeBuffers->available() / (float)numDataBufs) * 100;
	else
		return 100;
}

//SDRThreads
ProducerWorker::ProducerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
	msInterval = 1; //0 reserved by QTimer
	isRunning = false;
}

//This gets called by producerThread because we connected the started() signal to this method
void ProducerWorker::start()
{
	//Do any construction here so it's in the thread
	worker(cbProducerConsumerEvents::Start);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkNewData()));
	isRunning = true;
	timer->start(msInterval); //Times out when there's nothing in the event queue
	return; //Event loop will take over and our timer will fire worker function
}
void ProducerWorker::stop()
{
	isRunning = false;
	timer->stop();
	delete timer;
	//Do any worker destruction here
	worker(cbProducerConsumerEvents::Stop);
}
void ProducerWorker::checkNewData()
{
	if (!isRunning)
		return;
	//perform.StartPerformance("Producer");
	worker(cbProducerConsumerEvents::Run);
	//perform.StopPerformance(100);
}

ConsumerWorker::ConsumerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
	msInterval = 1;
	isRunning = false;
}

void ConsumerWorker::start()
{
	//Do any construction here so it's in the thread
	worker(cbProducerConsumerEvents::Start);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(processNewData()));
	isRunning = true;
	timer->start(msInterval); //Times out when there's nothing in the event queue
	return; //Event loop will take over and our timer will fire worker function
}
//Called just before thread finishes
void ConsumerWorker::stop()
{
	isRunning = false;
	timer->stop();
	delete timer;
	//Do any worker destruction here
	worker(cbProducerConsumerEvents::Stop);
}

//Consumer work can get executed 2 ways, as a result of signal from producer, or as a result of ConsumerWorker loop
//
void ConsumerWorker::processNewData()
{
	if (!isRunning)
		return;
	//perform.StartPerformance("Consumer");
	worker(cbProducerConsumerEvents::Run);
	//perform.StopPerformance(100);
}

