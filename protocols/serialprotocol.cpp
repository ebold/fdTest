#include "serialprotocol.h"

SerialProtocol::SerialProtocol()
{
    mDevice = 0;

    connect(this, SIGNAL(requestSend()), this, SLOT(startSend()));  // request to send protocol

    connect(&mTransmitTimeout, SIGNAL(timeout()), this, SLOT(checkSendQueue()));    // in case of missing response (no ack/nack)
    /*connect(this, SIGNAL(receivedACK()), this, SLOT(checkSendQueue()));     // action on positive response
    connect(this, SIGNAL(receivedNACK()), this, SLOT(checkSendQueue()));    // action on negative response*/
    mSerialDataRate = 38400;
    mSerialFrame = 1000 * 11;  // start + 8 data + 2 stop
    mTransmitTimeout.setInterval((100 * mSerialFrame)/mSerialDataRate + 1);   // estimated transmission duration up to 100 bytes (complete ASCII protocol) @ 38,4Kbps
}

void SerialProtocol::onDeviceDestroyed()
{
    mDevice = 0;
}

void SerialProtocol::setDevice(QIODevice* device)
{
    if(mDevice)
    {
        disconnect(mDevice,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
        disconnect(mDevice,SIGNAL(destroyed()),this,SLOT(onDeviceDestroyed()));
        mDevice = 0;
    }

    mDevice=device;

    if(mDevice)
    {
        connect(mDevice,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
        connect(mDevice,SIGNAL(destroyed()),this,SLOT(onDeviceDestroyed()));
    }
}

QIODevice* SerialProtocol::getDevice()
{
    return mDevice;
}

void SerialProtocol::onReadyRead()
{
    if(!mDevice)
        return;

    QByteArray b=mDevice->readAll();

    for(int i = 0; i < b.length(); i++)
    {
        mLastByte=b.at(i);

        receivedByte();

        switch(mLastByte)
        {
        case ACK:
            emit receivedACK();
            break;

        case NACK:
            emit receivedNACK();
            break;

        case EOT:
            emit receivedEOT();
            break;

        case STX:
            emit receivedSTX();
            break;

        case ETX:
            emit receivedETX();
            break;

        default:
            mRecvProtocol.append(mLastByte);
            break;
        }
    }
}

void SerialProtocol::sendProtocol(const QByteArray &protocol)
{
    if(!(mDevice && mDevice->isOpen()))
    {
        qWarning()<<"Tried to send protocol while disconnected";
        return;
    }

    mSendQueue.push_back(protocol);
    emit requestSend();
}

void SerialProtocol::checkSendQueue()
{
    QByteArray snd;

    if (mSendQueue.front().at(0) && (mSendQueue.front().at(1) & 0x80))  // msg started with special char, i.e., 'W'
    {
        snd = mSendQueue.front().mid(1);
        qDebug() << "serial protocol (hex) " << snd.toHex().toUpper();
    }
    else
    {
        snd = mSendQueue.front();
        qDebug() << "serial protocol (hex) " << snd.toHex();
    }

    emit sent(snd);
    mSendQueue.pop_front();

    mTransmitTimeout.stop();
    if(mSendQueue.isEmpty()) {
        emit nothingToSend();
    } else {
        emit requestSend();
    }
}

void SerialProtocol::startSend()
{
    int bytes = 1;

    if(!(mDevice && mDevice->isOpen()))
    {
        qWarning()<<"Tried to send protocol while disconnected";
        return;
    }

    if (!mTransmitTimeout.isActive())
    {
        QByteArray snd;
        char ctrlByte = 0;

        if (mSendQueue.front().at(0) && (mSendQueue.front().at(1) & 0x80))  // msg started with special char, i.e., 'W'
        {
            snd = mSendQueue.front().mid(1);
            ctrlByte = mSendQueue.front().at(0);
        }
        else
            snd = mSendQueue.front();

        char c=STX;
        mDevice->write(&c,1);
        ++bytes;

        if (ctrlByte)
        {
            mDevice->write(&ctrlByte, 1);
            ++bytes;
            mDevice->write(snd.toHex().toUpper());
        }
        else
            mDevice->write(snd.toHex());

        bytes += snd.toHex().length();

        c=ETX;
        mDevice->write(&c,1);
        ++bytes;

        mTransmitTimeout.setInterval((bytes * mSerialFrame)/mSerialDataRate + 1);
        mTransmitTimeout.start();
    }
}

void SerialProtocol::setSerialFrame(int frame)
{
    mSerialFrame = frame * 1000;
}

void SerialProtocol::setSerialDataRate(int rate)
{
    mSerialDataRate = rate;
}
