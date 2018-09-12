#ifndef FD_H
#define FD_H

namespace fd
{
    static const QString configFileName = "fdTest.conf";
    static const QString configOptionsFileName = "/fdConfigOptions.conf";
    static const QString testPatternsFileName = "/fdTestPatterns.conf";

    static const QString DEV_NAME_FD10 =  "FD10";
    static const QString DEV_NAME_FD15 =  "FD15";
    static const QString DEV_NAME_FD20 =  "FD20";

    static const uchar DEV_MAX_CHAR_FD10 =  10;
    static const uchar DEV_MAX_CHAR_FD15 =  15;
    static const uchar DEV_MAX_CHAR_FD20 =  20;

    static const uchar LR_CMD_26 = 0x26;
    static const uchar LR_CMD_27 = 0x27;
    static const uchar LR_CMD_28 = 0x28;

    static const int UI_PERIOD_BLINK = 500; // 500 ms
    static const int UI_SLIDING_CHAR_RATE = 237; // 237 ms
    static const int UI_SLIDING_START_DELAY = 2000; // 2 s
    static const int UI_SLIDING_HOLD_TIME = 1.9 * UI_SLIDING_START_DELAY; // 2 s + 0.9*2 s

    static const int CNT_VALID_ACK = 3;   // number of acknowledgements to be considered valid
    static const int PERIOD_TEXT = 10000; // 10-sec period for switching between multiple text

    static const QString FlurdisplaySection = "flurdisplay";
    static const QString DeviceSection = "device";
    static const QString FirmwareSection = "firmware";
    static const QString DevInterfaceSection = "devInterface";
    static const QString DevInterfaceDefName = "RS485";
    static const QString DevInterfaceDefParam = "38400,n,8,2";
    static const QString DevInterfaceDefCmd = "0x28";
    static const QString DevInterfaceCmd = "command";
    static const QString FlurdisplaySupportedCmds = "0x26,0x27,0x28";
    static const QString HostInterfaceSection = "hostInterface";
    static const QString HostInterfaceDefName = "COM4";
    static const QString ConfigName = "name";
    static const QString ConfigParam = "param";

    static const QString DevSection = "device";
    static const QString RulesSection = "rules";

    static const QString MainHomeAddr = "homeAddr";
    static const QString MainDisplayName = "displayName";
    static const QString MainSerial = "tty";
    static const QString MainSerialParams = "COM5,38400,n,8,2";

    static const QString DevMaxChar = "maxChar";
    static const QString DevTime = "time";
    static const QString DevTimeFormat = "H:mm";
    static const QString DevOnLongText = "onLongText";
    static const QString DevLongTextSliding = "sliding";
    static const QString DevLongTextSlidingCharRate = "slidingCharRate";
    static const QString DevLongTextSlidingHoldTime = "slidingHoldTime";

    static const QString RulesSupervisor = "supervisor";
    static const QString RulesExclusion = "excluded";
    static const QString RulesMatch = "match";
    static const QString RulesMatchEvent = "event";
    static const QString RulesMatchEventType = "type";
    static const QString RulesMatchPriority = "priority";
    static const QString RulesMatchBlink = "blink";
        static const QString RulesMatchBlinkNone = "none";
        static const QString RulesMatchBlinkAll = "all";
        static const QString RulesMatchBlinkEvent = "event";
        static const QString RulesMatchBlinkLocation = "location";
        static const QString RulesMatchBlinkDelimiter = "delimiter";
    static const QString RulesMatchTone = "tone";
        static const QString RulesMatchToneCall = "call";
        static const QString RulesMatchToneAlarm = "alarm";
        static const QString RulesMatchToneNone = "none";
    static const QString RulesMatchEventText = "eventText";
    static const QString RulesMatchLocationText = "locationText";
    static const QString RulesMatchIgnore = "ignore";
    static const QString RulesMatchIgnorePermission = "permission";
    static const QString RulesMatchIgnoreRemote = "remote";
    static const QString RulesMatchAcknowledged = "acknowledged";
    static const QString RulesMatchAddrList = "addrList";
    static const QString RulesMatchAddrListStations = "stations";
    static const QString RulesMatchAddrListSupervisors = "supervisors";

    // event names
    static const QString ReminderEvent ="reminder";
    static const QString CallEvent     ="call";
    static const QString AlarmEvent    ="alarm";
    static const QString PresenceEvent ="presence";
    static const QString PNAEvent      ="PNA";
    static const QString BMAEvent      ="BMA";
    static const QString T8Event       ="T8";
    static const QString UnknownEvent  ="?";

    // event text
    static const QString ReminderText ="Merk";
    static const QString CallText     ="Ruf";
    static const QString AlarmText    ="Alarm";
    static const QString PresenceText ="Anw";
    static const QString PNAText      ="PNA";
    static const QString BMAText      ="BMA";
    static const QString T8Text       ="T8";
    static const QString UnknownText  ="?";

    static const QString Alarm = "alarm";
    static const QString Presence = "presence";
    static const QString Reminder = "reminder";
    static const QString Prisoner = "prisoner";
    static const QString Officer = "officer";
    static const QString Bathroom = "bathroom";
    static const QString Sabotage = "sabotage";
    static const QString Fire = "fire";

    // protocol 0x28
    static const uchar PROT_HDR_SEND_ASW = 0;
    static const uchar PROT_HDR_CMD = 1;
    static const uchar PROT_28_DST_ST = 2;
    static const uchar PROT_28_DST_RM = 3;
    static const uchar PROT_28_SRC_ST = 4;
    static const uchar PROT_28_SRC_RM = 5;
    static const uchar PROT_28_DEV_TYPE = 6;
    static const uchar PROT_28_ST_GRP = 7;
    static const uchar PROT_28_RM_GRP = 8;
    static const uchar PROT_28_MSG_ID = 9;
    static const uchar PROT_28_TONE = 10;
    static const uchar PROT_28_TXT_FORMAT = 11;
    static const uchar PROT_28_TXT_COLOR = 12;
    static const uchar PROT_28_PRIORITY = 13;
    static const uchar PROT_28_PL_LENGTH = 14;
    static const uchar PROT_28_PL_DATA = 15;

    // protocol 0x26
    static const uchar PROT_26_GRP = 2;
    static const uchar PROT_26_PRIORITY = 3;
    static const uchar PROT_26_PL_DATA = 4;
    static const uchar PROT_26_PL_LEN = 8;
    static const uchar PROT_26_TONE_CALL = 10;  // leiser, tiefer Ton
    static const uchar PROT_26_TONE_ALARM = 0x80 | 0x40 | 0x0B; // lauter, höher Ton

    // protocol 0x27
    static const uchar PROT_27_GRP = 2;
    static const uchar PROT_27_PRIORITY = 3;
    static const uchar PROT_27_ALARM_TYPE = 4;
    static const uchar PROT_27_SPACE = 5;
    static const uchar PROT_27_PL_LEN = 7;

    // bit masks
    static const int PRTY_LOWEST = 1;
    static const int PRTY_TIME = 3;
    static const int PRTY_PRECENSE = 5;
    static const int PRTY_REMINDER = 7;
    static const int PRTY_CALL = 10;
    static const int PRTY_CALL_WC = 11;
    static const int PRTY_ALARM = 20;

    static const int TONE_NONE = 0x00;
    static const int TONE_CALL = 0x40;
    static const int TONE_ALARM = 0x80;

    static const int BLINK_ALL = 0x80;
    static const int BLINK_NONE = 0x00;

    static const int BLINK_EVENT = 0x40;
    static const int BLINK_LOCATION = 0x20;
    static const int BLINK_DELIMITER = 0x10;
    static const int BLINK_CHAR = 0x80;

    static const int ALIGN_LEFT = 0x20;
    static const int ALIGN_RIGHT = 0x10;
    static const int ALIGN_CENTER = 0x00;

    static const uchar SLIDING_TEXT = 0x40;
    static const uchar MULTIPLE_TEXT = 0x04;
}
#endif // FD_H

