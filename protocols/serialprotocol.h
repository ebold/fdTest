#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include <QObject>
#include <QList>
#include <QIODevice>
#include <QTimer>

#include <QDebug>

class SerialProtocol : public QObject
{
    Q_OBJECT
public:
    SerialProtocol();

    void setDevice(QIODevice* device = 0);
    QIODevice* getDevice();
    void setSerialFrame(int frame);
    void setSerialDataRate(int rate);

    static const char STX = 0x02;
    static const char ETX = 0x03;
    static const char EOT = 0x04;
    static const char ACK = 0x06;
    static const char NACK = 0x15;

signals:
    void receivedACK();
    void receivedNACK();
    void receivedSTX();
    void receivedETX();
    void receivedEOT();
    void receivedByte();
    void receivedNonControl();

    void sent(QByteArray byte);
    void requestSend();
    void nothingToSend();

public slots:
    void sendProtocol(const QByteArray &protocol);

private slots:
    void onReadyRead();
    void onDeviceDestroyed();
    void checkSendQueue();
    void startSend();

private:
    QList<QByteArray> mSendQueue;
    QIODevice* mDevice;
    char mLastByte;
    QByteArray mRecvProtocol;
    QTimer mTransmitTimeout;    // timer used to control transmission duration
    int mSerialDataRate;        // bits per second
    int mSerialFrame;           // bits per byte
};

#endif // SERIALPROTOCOL_H
