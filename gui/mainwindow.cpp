#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

using namespace fd;

static const char* UI_TXT_TONE_NONE = QT_TRANSLATE_NOOP("MainWindow", "none");
static const char* UI_TXT_TONE_CALL = QT_TRANSLATE_NOOP("MainWindow", "call");
static const char* UI_TXT_TONE_ALARM = QT_TRANSLATE_NOOP("MainWindow", "alarm");

static const char* UI_PROTOCOL_VIEW_RX = QT_TRANSLATE_NOOP("MainWindow", "RX: ");
static const char* UI_PROTOCOL_VIEW_TX = QT_TRANSLATE_NOOP("MainWindow", "tx: ");

MainWindow::MainWindow(QJsonObject configOptions, QJsonObject testPatterns, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mSetupWizard = new SetupWizard(configOptions);

    connect(mSetupWizard, SIGNAL(accepted()), this, SLOT(onSetupAccepted()));

    mSerialPort = new QSerialPort(this);
    connect(mSerialPort, SIGNAL(baudRateChanged(qint32,QSerialPort::Directions)), this, SLOT(adjustSerialDataRate()));
    connect(mSerialPort, SIGNAL(dataBitsChanged(QSerialPort::DataBits)), this, SLOT(adjustSerialFrame()));
    connect(mSerialPort, SIGNAL(parityChanged(QSerialPort::Parity)), this, SLOT(adjustSerialFrame()));
    connect(mSerialPort, SIGNAL(stopBitsChanged(QSerialPort::StopBits)), this, SLOT(adjustSerialFrame()));

    QFile configFile;

    configFile.setFileName(configFileName);

    if(configFile.exists())
    {
        QJsonParseError parseError;

        configFile.open(QFile::ReadOnly);
        QByteArray jsonData = configFile.readAll();
        configFile.close();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData,&parseError);

        if(parseError.error)
        {
            int line =jsonData.left(parseError.offset).count('\n');
            qDebug()<<"###########################################";
            qDebug()<<"Error in "+configFile.fileName()+" at line "<<line;
            qDebug()<<parseError.errorString();
            exit(1);
        }
        if (jsonDoc.isObject())
        {
            mCfgFlurdisplay = jsonDoc.object()[FlurdisplaySection].toObject();
            mCfgTest = jsonDoc.object()[RulesSection].toObject();

            configSerialPort(mCfgFlurdisplay[HostInterfaceSection].toObject()); // update host interface
        }
        else
            qDebug() << configFile.fileName() << " is not a JSON object";
    }
    else
    {
        mCfgTest = testPatterns[RulesSection].toObject();

        ui->startStopButton->setEnabled(false); // disable "Start/Stop" button

        QMessageBox::critical(this, tr("Configuration error"), tr("Set up configuration!"));
    }

    updateConfigurationLabel(mCfgFlurdisplay);  // update UI labels

    mSerialProtocol = new SerialProtocol;
    mSerialProtocol->setDevice(mSerialPort);

    mTestManager = new TestManager(mSerialProtocol, mCfgFlurdisplay, mCfgTest);

    //connect(mTestManager, SIGNAL(testStarted()), this, SLOT(onTestStarted()));
    //connect(mTestManager, SIGNAL(testStopped()), this, SLOT(onTestStopped()));
    connect(mTestManager, SIGNAL(protocolSent(QByteArray)), this, SLOT(onProtocolSent(QByteArray)));
    connect(mTestManager, SIGNAL(dummyProtocolSent()), this, SLOT(onDummyProtocolSent()));

    mBlinkTimer = new QTimer;
    mBlinkTimer->setInterval(UI_PERIOD_BLINK);
    connect(mBlinkTimer, SIGNAL(timeout()), this, SLOT(onBlinkTimerTimeout()));

    mSlidingTimer = new QTimer;
    mSlidingTimer->setInterval(UI_SLIDING_CHAR_RATE);
    connect(mSlidingTimer, SIGNAL(timeout()), this, SLOT(onSlidingTimerTimeout()));

    mSlidingDelayTimer = new QTimer;
    mSlidingDelayTimer->setSingleShot(true);
    mSlidingDelayTimer->setInterval(UI_SLIDING_START_DELAY);
    connect(mSlidingDelayTimer, SIGNAL(timeout()), this, SLOT(onSlidingDelayTimerTimeout()));

    // communication widget
    connect(mSerialProtocol, SIGNAL(receivedACK()), this, SLOT(showRxAck()));
    connect(mSerialProtocol, SIGNAL(receivedNACK()), this, SLOT(showRxNack()));
    connect(mSerialProtocol, SIGNAL(receivedSTX()), this, SLOT(showRxStx()));
    connect(mSerialProtocol, SIGNAL(receivedETX()), this, SLOT(showRxEtx()));
    connect(mSerialProtocol, SIGNAL(receivedEOT()), this, SLOT(showRxEot()));
    connect(mSerialProtocol, SIGNAL(sent(QByteArray)), this, SLOT(showTxBytes(QByteArray)));

    onTestStopped();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_configButton_clicked()
{
    if (mTestManager && mTestManager->isTestActive())
    {
        mTestManager->stop();
        onTestStopped();
    }

    if (mSetupWizard)
    {
        mSetupWizard->restart();
        mSetupWizard->show();
    }
}

void MainWindow::on_startStopButton_clicked()
{
    if (mTestManager)
    {
        if (mTestManager->isTestActive())
        {
            mTestManager->stop();
            onTestStopped();
        }
        else
        {
            if (mSerialPort->portName().isEmpty())
                if (configSerialPort(mSetupWizard->configDev()[HostInterfaceSection].toObject()))    // cannot configure port
                    return;

            if (openSerialPort())   // cannot open port
                return;
            onTestStarted();
            mTestManager->start();
        }
    }
}

void MainWindow::createConfigFile(QString fileName, QJsonObject &cfgDev, QJsonObject &cfgTestPattern)
{
    QFile configFile;
    configFile.setFileName(fileName);

    if (configFile.exists())
    {
        if (!configFile.remove())
        {
            qWarning() << "Cannot delete config file" << configFile.fileName();
            exit(2);
        }
    }

    if (!configFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot create config options file" << configFile.fileName();
        exit(2);
    }

    // store config to file in JSON format
    QJsonObject cfgObj;

    cfgObj[FlurdisplaySection] = cfgDev;
    cfgObj[RulesSection] = cfgTestPattern;
    configFile.write(QJsonDocument(cfgObj).toJson());
    configFile.close();
}

QJsonObject MainWindow::makeDefaultTestPatterns()
{
    QJsonObject cfgTest;
    QJsonArray rules;
    QJsonObject rule;
    QJsonObject list;

    // rules
    QString addrRange = "1-63";
    QMap<QString, QStringList> events;
    events["Zellenruf"] <<  Alarm << Prisoner <<                      // "event", "type"
                            QString::number(PRTY_CALL) << CallText <<   // "priority", "eventText"
                            RulesMatchBlinkNone << RulesMatchToneCall;  // "blink", "tone"
    events["WC-Ruf"] <<     Alarm << Bathroom <<
                            QString::number(PRTY_CALL_WC) << CallText <<
                            RulesMatchBlinkNone << RulesMatchToneCall;
    events["Beamtenalarm"] << Alarm << Officer <<
                            QString::number(PRTY_ALARM) << AlarmText <<
                            RulesMatchBlinkEvent << RulesMatchToneAlarm;
    events["Brandalarm"] << Alarm << Fire <<
                            QString::number(PRTY_ALARM) << AlarmText <<
                            RulesMatchBlinkEvent << RulesMatchToneAlarm;
    events["Deckelalarm"] << Alarm << Sabotage <<
                            QString::number(PRTY_ALARM) << AlarmText <<
                            RulesMatchBlinkEvent << RulesMatchToneAlarm;
    events["Beamtenanwesenheit"] << Presence << Officer <<
                            QString::number(PRTY_PRECENSE) << PresenceText <<
                            RulesMatchBlinkNone << RulesMatchToneNone;
    events["Merkschaltung"] << Reminder << Officer <<
                            QString::number(PRTY_REMINDER) << ReminderText <<
                            RulesMatchBlinkEvent << RulesMatchToneNone;

    foreach (QString key, events.keys())
    {
        rule["_ruleName"] = key + " in Stationen " + addrRange;
        rule[RulesMatchEvent] = events.value(key).at(0);
        rule[RulesMatchEventType] = events.value(key).at(1);

        rule[RulesMatchPriority] = events.value(key).at(2).toInt();
        rule[RulesMatchEventText] = events.value(key).at(3);
        rule[RulesMatchBlink] = events.value(key).at(4);
        rule[RulesMatchTone] = events.value(key).at(5);

        list[RulesMatchAddrListStations] = addrRange;
        rule[RulesMatchAddrList] = list;

        rules.append(rule);
    }

    cfgTest[RulesMatch] = rules;

    return cfgTest;
}

QJsonObject MainWindow::makeDefaultDeviceConfig()
{
    QJsonObject cfgFlurdisplay;

    QJsonObject configDevice;
    // device type
    configDevice[ConfigName] = DEV_NAME_FD10;
    configDevice[DevMaxChar] = DEV_MAX_CHAR_FD10;
    configDevice[DevTime] = DevTimeFormat;
    configDevice[DevOnLongText] = DevLongTextSliding +
            "," + QString::number(UI_SLIDING_CHAR_RATE) + "," + QString::number(UI_SLIDING_HOLD_TIME);
    cfgFlurdisplay[DevSection] = configDevice;

    // device firmware
    QJsonObject configFirmware;
    configFirmware[ConfigName] = "FD-J-03 or later";
    configFirmware["major"] = 3;
    cfgFlurdisplay[FirmwareSection] = configFirmware;

    // device interface
    QJsonObject configInterface;
    configInterface[ConfigName] = DevInterfaceDefName;
    configInterface[ConfigParam] = DevInterfaceDefParam;
    configInterface[DevInterfaceCmd] = DevInterfaceDefCmd;
    cfgFlurdisplay[DevInterfaceSection] = configInterface;

    // host interface
    configInterface[ConfigName] = HostInterfaceDefName;
    configInterface[ConfigParam] = DevInterfaceDefParam;    // must be the same with device interface
    cfgFlurdisplay[HostInterfaceSection] = configInterface;

    return cfgFlurdisplay;
}

// return true on errors
bool MainWindow::configSerialPort(QJsonObject config)
{
    // serial port settings
    if (config.isEmpty())
    {
        QMessageBox::critical(this, tr("Configuration error"), "Serial param is not defined!");
        return true;
    }

    if (config.contains(ConfigName) && config[ConfigName].isString())
    {
        mSerialPort->setPortName(config[ConfigName].toString());

        if (mSerialPort->portName().isEmpty())
        {
            QMessageBox::critical(this, tr("Bad parameter"), "No serial port name!");
            return true;
        }
    }

    QStringList serialParams;

    if (config.contains(ConfigParam) && config[ConfigParam].isString())
    {
        serialParams = config[ConfigParam].toString().split(",");

        if(serialParams.count() != 4)
        {
            QMessageBox::critical(this, tr("Bad parameter"), config[ConfigParam].toString());
            return true;
        }
    }

    mSerialPort->setBaudRate(serialParams.at(0).toInt());

    if(serialParams.at(1) == "n")
        mSerialPort->setParity(QSerialPort::NoParity);
    else if(serialParams.at(1) == "o")
        mSerialPort->setParity(QSerialPort::OddParity);
    else if(serialParams.at(1) == "e")
        mSerialPort->setParity(QSerialPort::EvenParity);
    else
        QMessageBox::critical(this, tr("Error"), tr("parity should be n or o or e"));

    mSerialPort->setDataBits((QSerialPort::DataBits)serialParams.at(2).toInt());
    mSerialPort->setStopBits((QSerialPort::StopBits)serialParams.at(3).toInt());

    qDebug() << "<name>" << mSerialPort->portName() << "<speed>,<parity>,<databits>,<stopbits>" << serialParams;

    return false;
}

// return true on errors
bool MainWindow::openSerialPort()
{
    mSerialPort->open(QSerialPort::ReadWrite);

    if(mSerialPort->isOpen())
    {
        mSerialPort->readAll();
        return false;
    }
    else
        QMessageBox::critical(this, tr("Error"), tr("Failed to open serial port ") + mSerialPort->portName());

    return true;
}

void MainWindow::closeSerialPort()
{
    if (mSerialPort->isOpen())
        mSerialPort->close();
}

void MainWindow::onTestStarted()
{
    ui->startStopButton->setText(tr("St&op"));

    if (!ui->testPassedButton->isEnabled())
        ui->testPassedButton->setEnabled(true);

    if (!ui->testFailedButton->isEnabled())
        ui->testFailedButton->setEnabled(true);

    ui->protocolView->clear();
}

void MainWindow::onTestStopped()
{
    mBlinkTimer->stop();
    mSlidingDelayTimer->stop();
    mSlidingTimer->stop();
    ui->ledDisplay->clear();
    ui->indicatorMultipleText->clear();

    ui->startStopButton->setText(tr("St&art"));

    if (ui->testPassedButton->isEnabled())
        ui->testPassedButton->setEnabled(false);

    if (ui->testFailedButton->isEnabled())
        ui->testFailedButton->setEnabled(false);

    mCurrTone = UI_TXT_TONE_NONE;
    ui->labelTone->setText(QApplication::translate("MainWindow",mCurrTone));
}

void MainWindow::onSetupAccepted()
{
    mCfgFlurdisplay = mSetupWizard->configDev();    // get current configuration

    configSerialPort(mCfgFlurdisplay[HostInterfaceSection].toObject()); // update host interface

    updateConfigurationLabel(mCfgFlurdisplay);  // update GUI

    createConfigFile(configFileName, mCfgFlurdisplay, mCfgTest);    // update conf file

    if (mTestManager->init(mCfgFlurdisplay, mCfgTest))  // update test manager
        qDebug() << "test manager cannot inited!";

    if (!ui->startStopButton->isEnabled())              // enable test start button
        ui->startStopButton->setEnabled(true);
}

void MainWindow::updateConfigurationLabel(QJsonObject config)
{
    if (config[DeviceSection].toObject()[ConfigName].isString())
        ui->labelDevice->setText(tr("Device type: ") + config[DeviceSection].toObject()[ConfigName].toString());

    if (config[FirmwareSection].toObject()[ConfigName].isString())
        ui->labelFirmware->setText(tr("Device FW: ") + config[FirmwareSection].toObject()[ConfigName].toString());

    if (config[DevInterfaceSection].toObject()[ConfigName].isString())
        ui->labelDevInterface->setText(tr("Device interface: ") + config[DevInterfaceSection].toObject()[ConfigName].toString());

    if (config[DevInterfaceSection].toObject()[ConfigParam].isString())
        ui->labelDevInterfaceParam->setText(tr("Device interface param: ") + config[DevInterfaceSection].toObject()[ConfigParam].toString());

    if (config[HostInterfaceSection].toObject()[ConfigName].isString())
        ui->labelHostInterface->setText(tr("Host interface: ") + config[HostInterfaceSection].toObject()[ConfigName].toString());

    if (config[DeviceSection].toObject()[DevMaxChar].isDouble())
    {
        ui->ledDisplay->setMaxLength(config[DeviceSection].toObject()[DevMaxChar].toInt());

        QString empty;
        empty.fill(' ', config[DeviceSection].toObject()[DevMaxChar].toInt());
        ui->ledDisplay->setText(empty);
        ui->ledDisplay->setFixedWidth(ui->ledDisplay->sizeHint().width());
    }
}

void MainWindow::onProtocolSent(QByteArray byte)
{
    QByteArray frontText;
    QByteArray backText;
    char ch;
    bool isBlinking = false;

    if (!mTestManager->isTestActive())
        return;

    if (byte == mSentProtocol)
        return;

    mSentProtocol = byte;
    mSlidingTimer->stop();

    // extract text from byte protocol
    if (byte.at(PROT_HDR_CMD) == LR_CMD_28)
    {
        if (byte.length() > (PROT_28_PL_DATA + byte.at(PROT_28_PL_LENGTH)))
        {
            frontText = byte.mid(PROT_28_PL_DATA, byte.at(PROT_28_PL_LENGTH));
            backText = frontText;
        }

        for (int i = 0; i < frontText.length(); ++i)
        {
            if (frontText.at(i) & BLINK_CHAR)
            {
                ch = frontText.at(i) & ~BLINK_CHAR;
                frontText[i] = ch;

                backText[i] = QChar::Space;
                isBlinking = true;
            }
        }

        if (byte.at(PROT_28_TXT_FORMAT) & MULTIPLE_TEXT)
            ui->indicatorMultipleText->setText(":");
        else
            ui->indicatorMultipleText->clear();

        if (byte.at(PROT_28_TXT_FORMAT) & SLIDING_TEXT)
        {
            mSlidingDelayTimer->start();
            mSlidingAndBlinking = isBlinking;
        }

        mCurrTone = UI_TXT_TONE_NONE;
        if (byte.at(PROT_28_TONE) & TONE_CALL)
            mCurrTone = UI_TXT_TONE_CALL;
        else if (byte.at(PROT_28_TONE) & TONE_ALARM)
            mCurrTone = UI_TXT_TONE_ALARM;
    }
    else if ((byte.at(PROT_HDR_CMD) == LR_CMD_26) ||
             (byte.at(PROT_HDR_CMD) == LR_CMD_27))
    {

        frontText = byte.mid(PROT_26_PL_DATA, PROT_26_PL_LEN);
        backText = frontText;

        if (byte.at(PROT_HDR_CMD) == LR_CMD_26)
        {
            for (int i = 0; i < frontText.length(); ++i)
            {
                if (frontText.at(i) & BLINK_CHAR)
                {
                    ch = frontText.at(i) & ~BLINK_CHAR;
                    frontText[i] = ch;

                    backText[i] = QChar::Space;
                    isBlinking = true;
                }
            }
        }
        else if (byte.at(PROT_HDR_CMD) == LR_CMD_27)
        {
            // blinking all or not
            if (frontText.at(0) & BLINK_CHAR)
            {
                isBlinking = true;

                for (int i = 0; i < frontText.length(); ++i)
                {
                    if (frontText.at(i) & BLINK_CHAR)
                    {
                        ch = frontText.at(i) & ~BLINK_CHAR;
                        frontText[i] = ch;
                    }

                    backText[i] = QChar::Space;
                }
            }

            // insert extra space between Rufart and Adress for command 0x27
            QByteArray temp = frontText;

            frontText = temp.left(1);
            frontText.append(QChar::Space);
            frontText.append(temp.mid(1));

            temp = backText;

            backText = temp.left(1);
            backText.append(QChar::Space);
            backText.append(temp.mid(1));
        }

        mCurrTone = UI_TXT_TONE_NONE;
        uchar tone = byte.at(PROT_26_PRIORITY);
        if (tone == PROT_26_TONE_CALL)
            mCurrTone = UI_TXT_TONE_CALL;
        else if (tone > PROT_26_TONE_CALL)
            mCurrTone = UI_TXT_TONE_ALARM;
    }

    // display text
    mDisplayText.clear();
    mCurrDisplayText = QString::fromUtf8(frontText);
    mDisplayText.append(mCurrDisplayText);
    if (isBlinking)
    {
        mDisplayText.append(QString::fromUtf8(backText));

        if (!mBlinkTimer->isActive())
            mBlinkTimer->start();
    }
    else
    {
        if(mBlinkTimer->isActive())
            mBlinkTimer->stop();
    }

    ui->ledDisplay->setText(mCurrDisplayText);

    ui->labelTone->setText(QApplication::translate("MainWindow",mCurrTone));
}

void MainWindow::onBlinkTimerTimeout()
{
    QString text = mCurrDisplayText;

    if (mDisplayText.first().contains(text))
        mCurrDisplayText = mDisplayText.last();
    else
        mCurrDisplayText = mDisplayText.first();

    ui->ledDisplay->setText(mCurrDisplayText);
}

void MainWindow::onSlidingDelayTimerTimeout()
{
    if (!mSlidingTimer->isActive())
    {
        mSlidingTimer->start();
        mSlidingLength = mCurrDisplayText.length();

        if (mSlidingAndBlinking)
        {
            if(mBlinkTimer->isActive())
                mBlinkTimer->stop();

            mCurrDisplayText = mDisplayText.first();
            ui->ledDisplay->setText(mCurrDisplayText);
        }
    }
}

void MainWindow::onSlidingTimerTimeout()
{
    QString text;

    if (mDisplayText.size())
    {
        --mSlidingLength;

        if (mSlidingLength)
        {
            text = mCurrDisplayText.right(mSlidingLength);
        }
        else
        {
            text = mDisplayText.first();
            mSlidingLength = text.length();
            mSlidingTimer->stop();
            mSlidingDelayTimer->start();

            if (mSlidingAndBlinking)
            {
                if (!mBlinkTimer->isActive())
                    mBlinkTimer->start();
            }
        }

        mCurrDisplayText = text;
        ui->ledDisplay->setText(mCurrDisplayText);
    }
}

void MainWindow::onDummyProtocolSent()
{
    if (!mTestManager->isTestActive())
    {
        closeSerialPort();
        mSentProtocol.clear();
    }
}

void MainWindow::adjustSerialFrame()
{
    int frame = 0; // bits per byte
    if (mSerialProtocol && mSerialPort)
    {
        if (mSerialPort->dataBits() > 0)
            frame += mSerialPort->dataBits();
        else
            return;

        if (mSerialPort->parity() > 0)
            ++frame;

        if (mSerialPort->stopBits() > 0)
        {
            ++frame;
            if (mSerialPort->stopBits() != QSerialPort::OneStop)
                ++frame;
        }
        else
            return;

        mSerialProtocol->setSerialFrame(frame);
    }
}

void MainWindow::adjustSerialDataRate()
{
    if (mSerialProtocol && mSerialPort)
    {
        mSerialProtocol->setSerialDataRate(mSerialPort->baudRate());
    }
}

void MainWindow::showRxBytes(QString str)
{
    ui->protocolView->insertPlainText(UI_PROTOCOL_VIEW_RX + str + "\n");
}

void MainWindow::showRxAck()
{
    //showRxBytes(QByteArray::number(SerialProtocol::ACK));
    showRxBytes("ACK");
}

void MainWindow::showRxNack()
{
    showRxBytes("NACK");
}

void MainWindow::showRxStx()
{
    showRxBytes("STX");
}

void MainWindow::showRxEtx()
{
    showRxBytes("ETX");
}

void MainWindow::showRxEot()
{
    showRxBytes("EOT");
}

void MainWindow::showTxBytes(QByteArray bytes)
{
    ui->protocolView->insertPlainText(UI_PROTOCOL_VIEW_TX + bytes.toHex() + "\n");
}
