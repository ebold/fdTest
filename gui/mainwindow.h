#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QTimer>
#include <QDebug>

#include "fd.h"
#include "serialprotocol.h"
#include "testmanager.h"
#include <QDialog>
#include "setupwizard.h"

#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QJsonObject configOptions, QJsonObject testPatterns, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_configButton_clicked();
    void on_startStopButton_clicked();
    void onTestStarted();
    void onTestStopped();
    void onSetupAccepted();
    void onProtocolSent(QByteArray byte);
    void onBlinkTimerTimeout();
    void onSlidingTimerTimeout();
    void onSlidingDelayTimerTimeout();
    void onDummyProtocolSent();
    void adjustSerialFrame();
    void adjustSerialDataRate();
    void showRxAck();
    void showRxNack();
    void showRxStx();
    void showRxEtx();
    void showRxEot();
    void showTxBytes(QByteArray bytes);
    void showRxBytes(QString str);

private:
    Ui::MainWindow *ui;
    SetupWizard *mSetupWizard;
    TestManager *mTestManager;
    QSerialPort *mSerialPort;
    SerialProtocol *mSerialProtocol;

    QJsonObject mCfgFlurdisplay;      // flurdisplay settings
    QJsonObject mCfgTest;    // test patterns

    QJsonObject makeDefaultDeviceConfig();
    QJsonObject makeDefaultTestPatterns();
    void createConfigFile(QString fileName, QJsonObject &cfgDev, QJsonObject &cfgTestPattern);

    bool configSerialPort(QJsonObject config);
    bool openSerialPort();
    void closeSerialPort();

    void updateConfigurationLabel(QJsonObject config);
    QTimer *mBlinkTimer;
    QStringList mDisplayText;
    QString mCurrDisplayText;   // text on GUI display
    QByteArray mSentProtocol;
    const char* mCurrTone;          // tone indicator on GUI
    QTimer *mSlidingTimer;
    QTimer *mSlidingDelayTimer;
    int mSlidingLength;
    bool mSlidingAndBlinking;   // sliding with blinking, only for 0x28 command
};

#endif // MAINWINDOW_H
