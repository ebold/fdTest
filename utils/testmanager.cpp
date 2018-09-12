#include "testmanager.h"
#include <QDebug>
#include <fd.h>

using namespace fd;

TestManager::TestManager(SerialProtocol *protocol, QJsonObject config, QJsonObject test, QObject *parent) :
    QObject(parent)
{
    // internal settings
    mSettings[DevMaxChar] = DEV_MAX_CHAR_FD10;
    mSettings[DevOnLongText] = 0;
    mSettings[DevLongTextSlidingCharRate] = UI_SLIDING_CHAR_RATE;
    mSettings[DevLongTextSlidingHoldTime] = UI_SLIDING_HOLD_TIME;
    mSettings[DevInterfaceCmd] = QString::number(LR_CMD_28);

    mSerialProtocol = protocol;

    connect(mSerialProtocol, SIGNAL(receivedACK()), this, SLOT(onReceivedACK()));
    connect(mSerialProtocol, SIGNAL(sent(QByteArray)), this, SLOT(onSent(QByteArray)));

    mIsTestActive = false;

    if (init(config, test))
        qWarning() << "cannot init test manager!";

    // setup LR protocol
    buildLrProtocolHeader(LR_CMD_28);
    mCntAck = 0;
}

TestManager::~TestManager()
{

}

// return true on errors
bool TestManager::init(QJsonObject config, QJsonObject test)
{
    if (config.contains(DevSection) && config[DevSection].isObject())
        mConfigDevice = config[DevSection].toObject();

    if (mConfigDevice.isEmpty())
        return true;

    if (mConfigDevice.contains(DevMaxChar) && mConfigDevice[DevMaxChar].isDouble() &&
            (mConfigDevice[DevMaxChar].toInt() > 0))
        mSettings[DevMaxChar] = mConfigDevice[DevMaxChar].toInt();
    if (mConfigDevice.contains(DevTime) && mConfigDevice[DevTime].isString())
        mSettings[DevTime] = mConfigDevice[DevTime].toString();
    if (mConfigDevice.contains(DevOnLongText) && mConfigDevice[DevOnLongText].isString())
    {
        QStringList sParamLongText = mConfigDevice[DevOnLongText].toString().split(",");

        if (!sParamLongText.at(0).isEmpty())
        {
            if (sParamLongText.at(0) == DevLongTextSliding)
            {
                mSettings[DevOnLongText] = SLIDING_TEXT;

                if (!sParamLongText.at(1).isEmpty())
                    mSettings[DevLongTextSlidingCharRate] = sParamLongText.at(1).toInt();

                if (!sParamLongText.at(2).isEmpty())
                    mSettings[DevLongTextSlidingHoldTime] = sParamLongText.at(2).toInt();
            }
        }
    }

    if (config.contains(DevInterfaceSection) && config[DevInterfaceSection].isObject())
        mConfigInterface = config[DevInterfaceSection].toObject();

    if (mConfigInterface.isEmpty())
        return true;

    if (mConfigInterface.contains(ConfigName) && mConfigInterface[ConfigName].isString())
        mSettings["interfaceName"] = mConfigInterface[ConfigName].toString();

    if (mConfigInterface.contains(DevInterfaceCmd) && mConfigInterface[DevInterfaceCmd].isString())
    {
        QStringList commands = mConfigInterface[DevInterfaceCmd].toString().split(",");
        foreach (QString c, commands)
            c.replace(QRegExp("\\s*"), ""); // remove space

        mSettings[DevInterfaceCmd] = commands;
    }

    if (config.contains(FirmwareSection) && config[FirmwareSection].isObject())
        mConfigFirmware = config[FirmwareSection].toObject();

    mConfig = config;

    createTestPatterns(test);

    qDebug() << mSettings[DevMaxChar] << mSettings["interfaceName"] << mSettings[DevInterfaceCmd];
    qDebug() << mConfig[HostInterfaceSection].toObject()[ConfigName].toString() <<
                mConfig[HostInterfaceSection].toObject()[ConfigParam].toString();
    return false;
}

bool TestManager::start()
{
    if (mTestPatterns.isEmpty())
    {
        qDebug() << "Test patterns are not defined in config file!";
        mIsTestActive = false;
    }
    else
    {
        mIntervalTimer = startTimer(500);
        mCurrTestPattern = mTestPatterns.last();
        mIsTestActive = true;
        emit testStarted();
    }

    return mIsTestActive;
}

bool TestManager::stop()
{
    if (mIntervalTimer)
    {
        killTimer(mIntervalTimer);
        mIntervalTimer = 0;
    }
    mIsTestActive = false;

    // send dummy pattern
    mDummyProtocol = buildLrProtocol(mDummyPattern, mProtocolHeader);

    if (!mDummyProtocol.isEmpty())
    {
        if (mSettings["interfaceName"].toString().contains("Seriobus"))
        {
            QByteArray msgToT8 = mDummyProtocol;
            msgToT8.prepend('W');

            mSerialProtocol->sendProtocol(msgToT8);
            mSerialProtocol->sendProtocol(msgToT8);
            mSerialProtocol->sendProtocol(msgToT8);
            mCntAck = 0;
        }
        else
        {
            mSerialProtocol->sendProtocol(mDummyProtocol);
            mCntAck = CNT_VALID_ACK;
        }
    }

    emit testStopped();
    return true;
}

void TestManager::timerEvent(QTimerEvent *evt)
{
    if(evt->timerId() == mIntervalTimer)
    {
        sTestPattern testPattern = getPatternNextTo(mCurrTestPattern);

        display(testPattern);

        restartIntervalTimer(PERIOD_TEXT);
    }
}

void TestManager::restartIntervalTimer(int ival)
{
    if (mTestPatterns.length() > 1)    // start interval timer if at least 2 test patterns exist
    {
        mIntervalTimer = startTimer(ival);
    }
    else if (mIntervalTimer)
    {
        killTimer(mIntervalTimer);
        mIntervalTimer = 0;
    }
}

void TestManager::createTestPatterns(QJsonObject &match)
{
    mTestPatterns.clear();

    //if (config.contains(RulesSection) && config[RulesSection].isObject())
    {
        //QJsonObject match = config[RulesSection].toObject();

        if (match.contains(RulesMatch) && match[RulesMatch].isArray())
        {
            QJsonArray matchRules = match[RulesMatch].toArray();
            sTestPattern testPattern;
            testPattern.id = 0;

            qDebug() << "id" << "evtName" << "evtType" << "evtTxt" <<
                        "locTxt" << "prio" << "blink" << "tone";
            QJsonArray::Iterator itr;
            for (itr = matchRules.begin(); itr != matchRules.end(); ++itr)
            {
                QJsonObject rule;

                if ((*itr).isObject())
                {
                    rule = (*itr).toObject();

                    ++testPattern.id;
                    testPattern.evtName.clear();
                    testPattern.evtType.clear();
                    testPattern.evtTxt.clear();
                    testPattern.locTxt.clear();
                    testPattern.prio = 0;
                    testPattern.blink = 0;
                    testPattern.tone = 0;

                    if (rule.contains(RulesMatchEvent) && rule[RulesMatchEvent].isString())
                        testPattern.evtName = rule[RulesMatchEvent].toString();
                    if (rule.contains(RulesMatchEventType) && rule[RulesMatchEventType].isString())
                        testPattern.evtType = rule[RulesMatchEventType].toString();
                    if (rule.contains(RulesMatchEventText) && rule[RulesMatchEventText].isString())
                        testPattern.evtTxt = rule[RulesMatchEventText].toString();
                    if (rule.contains(RulesMatchLocationText) && rule[RulesMatchLocationText].isString())
                        testPattern.locTxt = rule[RulesMatchLocationText].toString();
                    if (testPattern.locTxt.isEmpty())
                    {
                        if (!testPattern.evtType.isEmpty())
                            testPattern.locTxt = testPattern.evtType;
                        else
                            testPattern.locTxt = QString::number(testPattern.id);
                    }
                    if (rule.contains(RulesMatchPriority) && rule[RulesMatchPriority].isDouble())
                        testPattern.prio = rule[RulesMatchPriority].toInt();
                    if (rule.contains(RulesMatchBlink) && rule[RulesMatchBlink].isString())
                    {
                        QStringList strBlink = rule[RulesMatchBlink].toString().split(',');

                        foreach (QString str, strBlink)
                            str.replace(QRegExp("\\s*"), "");   // remove space

                        testPattern.blink = BLINK_NONE;

                        if (strBlink.contains(RulesMatchBlinkNone))    // "none"
                            testPattern.blink = BLINK_NONE;
                        else if (strBlink.contains(RulesMatchBlinkAll))   // "all"
                            testPattern.blink = BLINK_ALL;
                        {
                            if (strBlink.contains(RulesMatchBlinkEvent))    // "event"
                                testPattern.blink = BLINK_EVENT;
                            if (strBlink.contains(RulesMatchBlinkLocation))    // "location"
                                testPattern.blink = BLINK_LOCATION;
                            if (strBlink.contains(RulesMatchBlinkDelimiter))    // "delimiter"
                                testPattern.blink = BLINK_DELIMITER;
                        }
                    }
                    if (rule.contains(RulesMatchTone) && rule[RulesMatchTone].isString())
                    {
                        if (rule[RulesMatchTone].toString() == RulesMatchToneCall)    // "call"
                            testPattern.tone = TONE_CALL;
                        else if (rule[RulesMatchTone].toString() == RulesMatchToneAlarm)    // "alarm"
                            testPattern.tone = TONE_ALARM;
                        else if (rule[RulesMatchTone].toString() == RulesMatchToneNone) // "none"
                            testPattern.tone = TONE_NONE;
                    }

                    mTestPatterns.append(testPattern);

                    qDebug() << testPattern.id << testPattern.evtName << testPattern.evtType << testPattern.evtTxt <<
                                testPattern.locTxt << testPattern.prio << testPattern.blink << testPattern.tone;
                }
            }
        }
    }
}

sTestPattern TestManager::getPatternNextTo(sTestPattern &actual)
{
    sTestPattern testPattern;

    if (!mTestPatterns.isEmpty())
    {
        int i = actual.id;

        if (i == mTestPatterns.length())
        {
            i = 0;    // 1st element
            bool ok;
            int cmd;

            // prepare LR protocol header (currently depends on the device interface)
            QStringList commands = mSettings[DevInterfaceCmd].toStringList();
            QString currentCmd = "0x";
            currentCmd.append(QString::number(mProtocolHeader.at(PROT_HDR_CMD), 16));

            QString nextCmd;
            if (commands.contains(currentCmd))
            {
                int i = commands.indexOf(currentCmd);
                ++i;
                if (i < commands.count())
                    nextCmd = commands.at(i);
                else
                    nextCmd = commands.first();
            }
            else
            {
                nextCmd = commands.first();
            }

            cmd = nextCmd.toInt(&ok, 16);
            if (ok)
                buildLrProtocolHeader(cmd);
        }

        testPattern = mTestPatterns.at(i);
    }

    return testPattern;
}

QByteArray TestManager::buildLrProtocol(sTestPattern &testPattern, QByteArray &header)
{
    QByteArray protocol;
    QByteArray protocolData;
    QByteArray array;

    if (header.at(PROT_HDR_CMD) == LR_CMD_28)
    {
        protocol.append(header);

        uchar crc = 0;

        // event text
        array = testPattern.evtTxt.toUtf8();

        if (!array.isEmpty() && (testPattern.blink & BLINK_EVENT))
        {
            char ch;
            for (int i = 0; i < array.length(); ++i)
            {
                ch = array.at(i) | BLINK_CHAR;
                array[i] = ch;
            }
        }
        protocolData.append(array);

        // delimiter
        if (!array.isEmpty())   // if event text is available, then insert delimiter between event and location texts
        {
            uchar delimiter = QChar::Space; // default delimiter

            if (testPattern.blink & BLINK_DELIMITER)
                delimiter |= BLINK_CHAR;

            protocolData.append(delimiter);
        }

        // location text
        array = testPattern.locTxt.toUtf8();

        QByteArray spaces;

        if (testPattern.evtName.isEmpty())  // blank
        {
            int empty = mSettings[DevMaxChar].toInt() - array.length() - protocolData.length();
            if (empty >= 0)
            {
                spaces.fill(QChar::Space, empty);
                array.prepend(spaces);
            }
        }
        else if (testPattern.evtName == DevTime)
        {
            int posColon = array.indexOf(':');

            char ch;
            for (int i = 0; i < array.length(); ++i)
            {
                if (array.at(i) == ':')
                {
                    ch = array.at(i) | BLINK_CHAR;
                    array[i] = ch;
                }
            }
            if (posColon > 0)
            {
                ++posColon;
                int empty = (mSettings[DevMaxChar].toInt() >> 1) - posColon - protocolData.length();
                if (empty > 0)
                {
                    spaces.fill(QChar::Space, empty);
                    array.prepend(spaces);
                }
            }
        }
        else if (!array.isEmpty() && (testPattern.blink & BLINK_LOCATION))
        {
            char ch;
            for (int i = 0; i < array.length(); ++i)
            {
                ch = array.at(i) | BLINK_CHAR;
                array[i] = ch;
            }
        }
        protocolData.append(array);

        // text type, protocol length
        quint8 textType = ALIGN_LEFT;

        if (testPattern.blink == BLINK_ALL)
            textType |= BLINK_ALL;

        protocol[PROT_28_PL_LENGTH] = protocolData.length();

        mSettings["intervalText"] = PERIOD_TEXT;

        if (protocolData.length() > mSettings[DevMaxChar].toInt())
        {
            if (mSettings[DevOnLongText].toInt() == SLIDING_TEXT)
            {
                textType |= SLIDING_TEXT;
                mSettings["intervalText"] = protocolData.length() * mSettings[DevLongTextSlidingCharRate].toInt() + mSettings[DevLongTextSlidingHoldTime].toInt();
            }
            else
                protocol[PROT_28_PL_LENGTH] = mSettings[DevMaxChar].toInt();
        }

        if (!(mCurrTestPattern.id == testPattern.id)) // if current & new items are different, then set "multiple text" bit in protocol
        {
            if (hasItemWithEqualPriority(testPattern))
                textType |= MULTIPLE_TEXT;
        }

        protocol[PROT_28_TXT_FORMAT] = textType;

        // tone
        protocol[PROT_28_TONE] = testPattern.tone;

        // crc
        for (int i = 0; i < protocolData.length(); ++i)
            crc ^= protocolData.at(i);

        protocolData.append(crc);
        protocol.append(protocolData);
    }
    else if ((header.at(PROT_HDR_CMD) == LR_CMD_26) ||
             (header.at(PROT_HDR_CMD) == LR_CMD_27))
    {
        protocol.append(header);

        // valence (Wertigkeit)
        char valence = 0;

        if (testPattern.tone == TONE_CALL)
            valence |= PROT_26_TONE_CALL;
        else if (testPattern.tone == TONE_ALARM)
            valence |= PROT_26_TONE_ALARM;

        protocol.append(valence);

        if (header.at(PROT_HDR_CMD) == LR_CMD_26)
        {
            // event text (max 3 chars)
            if (testPattern.evtTxt.length() > 3)
                array = testPattern.evtTxt.left(3).toUtf8();
            else
                array = testPattern.evtTxt.toUtf8();

            if (!array.isEmpty() && (testPattern.blink & BLINK_EVENT))
            {
                char ch;
                for (int i = 0; i < array.length(); ++i)
                {
                    ch = array.at(i) | BLINK_CHAR;
                    array[i] = ch;
                }
            }

            protocolData.append(array);

            // delimiter
            if (!array.isEmpty())   // if event text is available, then insert delimiter between event and location texts
            {
                uchar delimiter = QChar::Space; // default delimiter

                if (testPattern.blink & BLINK_DELIMITER)
                    delimiter |= BLINK_CHAR;

                protocolData.append(delimiter);
            }

            // location text
            int rest = PROT_26_PL_LEN - protocolData.length();
            if (testPattern.locTxt.length() > rest)
            {
                array = testPattern.locTxt.left(rest).toUtf8();
            }
            else if (testPattern.locTxt.isEmpty())
            {
                array.clear();
                array.fill('?', rest);
            }
            else
            {
                array.clear();
                array.fill(QChar::Space, rest);
                array.prepend(testPattern.locTxt.toUtf8());
                array.resize(rest);
            }

            protocolData.append(array);
        }
        else
        {
            // event text (1 char)
            if (testPattern.evtTxt.length())
                array = testPattern.evtTxt.left(1).toUtf8();

            if (!array.isEmpty())
            {
                if (testPattern.blink & BLINK_EVENT)
                    array[0] = array.at(0) | BLINK_CHAR;

                protocolData.append(array);

                // location text
                int rest = PROT_27_PL_LEN - protocolData.length();
                if (testPattern.locTxt.length() > rest)
                {
                    array = testPattern.locTxt.left(rest).toUtf8();
                }
                else if (testPattern.locTxt.isEmpty())
                {
                    array.clear();
                    array.fill('?', rest);
                }
                else
                {
                    array.clear();
                    array.fill(QChar::Space, rest);
                    array.prepend(testPattern.locTxt.toUtf8());
                    array.resize(rest);
                }

                protocolData.append(array);
            }
        }

        if (protocolData.length())
            protocol.append(protocolData);
        else
        {
            array.clear();
            array.fill(QChar::Space, PROT_26_PL_LEN);
            protocol.append(array);
        }
    }

    return protocol;
}

void TestManager::display(sTestPattern &testPattern)
{
    bool isCmdSupported = false;
    bool ok;
    int cmd;

    foreach(QString s, mSettings[DevInterfaceCmd].toStringList())
    {
        cmd = s.toInt(&ok,16);
        if (ok)
        {
            if (cmd == mProtocolHeader.at(PROT_HDR_CMD))
                isCmdSupported = true;
        }
    }

    if (isCmdSupported)
    {
        QByteArray protocol = buildLrProtocol(testPattern, mProtocolHeader);

        if (!protocol.isEmpty())
        {
            if (mSettings["interfaceName"].toString().contains("Seriobus"))
            {
                mT8Packet = protocol;
                mT8Packet.prepend('W');
                mSerialProtocol->sendProtocol(mT8Packet);
                mLastProtocol = protocol;
                mCntAck = 0;
            }
            else
            {
                mSerialProtocol->sendProtocol(protocol);
                mLastProtocol = protocol;
                mCntAck = CNT_VALID_ACK;
            }

            mCurrTestPattern = testPattern;
        }
    }
}

bool TestManager::hasItemWithEqualPriority(sTestPattern &actual)
{
    quint64 idx = actual.id;
    int priority = actual.prio;

    for (int i = 0; i < mTestPatterns.length(); ++i)
    {
        if (mTestPatterns.at(i).id != idx)
            if (mTestPatterns.at(i).prio == priority)
                return true;    // found another item with the equal priority
    }

    return false;
}

void TestManager::buildLrProtocolHeader(uchar cmd)
{
    if (cmd == LR_CMD_28)
    {
        mProtocolHeader.clear();

        mProtocolHeader[PROT_HDR_SEND_ASW] = 0x81;  // sender ASW = 0x81
        mProtocolHeader[PROT_HDR_CMD] = cmd;  // command = 0x28
        mProtocolHeader[PROT_28_DST_ST] = 0x00;  // destination addr = 0.0
        mProtocolHeader[PROT_28_DST_RM] = 0x00;
        mProtocolHeader[PROT_28_SRC_ST] = 0x09;  // source addr = 9.9
        mProtocolHeader[PROT_28_DST_RM] = 0x09;
        mProtocolHeader[PROT_28_DEV_TYPE] = 0x01;  // destination device = Flurdisplays
        mProtocolHeader[PROT_28_ST_GRP] = 0x00;  // destination station group = 0
        mProtocolHeader[PROT_28_RM_GRP] = 0x00;  // destination room group = 0
        mProtocolHeader[PROT_28_MSG_ID] = 0x01;  // message ID = 1
        mProtocolHeader[PROT_28_TONE] = 0x00;  // tone = none
        mProtocolHeader[PROT_28_TXT_FORMAT] = 0x00;  // format = default
        mProtocolHeader[PROT_28_TXT_COLOR] = 0x00;  // color = default
        mProtocolHeader[PROT_28_PRIORITY] = 0x00;  // priority = low
        mProtocolHeader[PROT_28_PL_LENGTH] = 0x00;
    }
    else if ((cmd == LR_CMD_26) ||
             (cmd == LR_CMD_27))
    {
        mProtocolHeader.clear();

        mProtocolHeader[PROT_HDR_SEND_ASW] = 0x81; // sender ASW = 0x81
        mProtocolHeader[PROT_HDR_CMD] = cmd;
        mProtocolHeader[PROT_26_GRP] = 0; // group number = 0
    }
}

void TestManager::onReceivedACK()
{
    if (mLastProtocol.isEmpty())
        return;

    if (!mIsTestActive)
        return;

    if (mCntAck < CNT_VALID_ACK)
        ++mCntAck;

    if (mSettings["interfaceName"].toString().contains("Seriobus"))
    {
        mSerialProtocol->sendProtocol(mT8Packet);
    }
}

void TestManager::onSent(QByteArray byte)
{
    if (byte == mDummyProtocol)
    {
        if (mCntAck < CNT_VALID_ACK)
            ++mCntAck;

        if (mCntAck >= CNT_VALID_ACK)
        {
            emit dummyProtocolSent();
            mDummyProtocol.clear();
            mLastProtocol.clear();
        }
    }
    else if (mCntAck >= CNT_VALID_ACK)
    {
        emit protocolSent(mLastProtocol);
    }
}
