#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#include <QObject>
#include <serialprotocol.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimerEvent>
#include <QByteArray>
#include <QSharedPointer>
#include <QList>

struct sTestPattern {
    quint64 id;       // event id
    QString evtName;  // event name
    QString evtTxt;   // event text
    QString evtType;  // event type
    QString locTxt;   // location text
    QString addr;     // LR-address
    int prio;         // priority
    int tone;         // tone type
    int blink;        // blink mode
};

class TestManager : public QObject
{
    Q_OBJECT
public:
    explicit TestManager(SerialProtocol *protocol, QJsonObject config, QJsonObject test, QObject *parent = 0);
    ~TestManager();

    bool isTestActive() { return mIsTestActive; }
    bool start();
    bool stop();
    bool init(QJsonObject config, QJsonObject test);

    void buildLrProtocolHeader(uchar cmd);
    QByteArray buildLrProtocol(sTestPattern &testPattern, QByteArray &header);
signals:
    void testStarted();
    void testStopped();
    void protocolSent(QByteArray byte);
    void dummyProtocolSent();

public slots:
private slots:
    void onReceivedACK();
    void onSent(QByteArray byte);
private:

    QMap<QString, QVariant> mSettings;

    SerialProtocol *mSerialProtocol;
    QByteArray mProtocolHeader;
    QByteArray mProtocolSync;
    QByteArray mLastProtocol;   // in Seriobus interface, protocol is emitted on received ack
    QByteArray mT8Packet;   // in Seriobus interface, packet is sent serially on received ack
    QByteArray mDummyProtocol;  // sent on test stop

    QJsonObject mConfig;
    QJsonObject mConfigDevice;
    QJsonObject mConfigInterface;
    QJsonObject mConfigFirmware;

    QJsonObject mTest;

    int mIntervalTimer;

    void timerEvent(QTimerEvent *evt);
    void restartIntervalTimer(int ival);

    QList<sTestPattern> mTestPatterns;
    void createTestPatterns(QJsonObject &config);
    sTestPattern mCurrTestPattern;
    sTestPattern getPatternNextTo(sTestPattern &actual);
    void display(sTestPattern &testPattern);
    bool hasItemWithEqualPriority(sTestPattern &actual);
    bool mIsTestActive;
    sTestPattern mDummyPattern;
    int mCntAck;
};

#endif // TESTMANAGER_H
