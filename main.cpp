#include <mainwindow.h>
#include <QApplication>
#include <QDir>

/**
 * The application is used to test the functionality of Flurdisplay
 *
 * Author: Enkhbold Ochirsuren
 */

/**
 * Target device firmware: FD-J-<n>
 * Serial interface, parameter:
 *  - Seriobus, COM<n>, 19200/N/8/1
 *  - RS485, COM<n>, 38400/N/8/2
 * Test patterns:
 *  - refer table
 *
 * What to test | 26 | 27 | 28
 * ---------------------------
 * Seriobus     | y  | y  | y
 * RS485        | -  | -  | y
 * Tone gen     | y  | y  | y
 * Blinking     | y  | y  | y
 * Text length  | y  | y  | y
 * CRC          | -  | -  | y
 * Sliding eff  | -  | -  | y
 * Double point | -  | -  | y
 *
 */
using namespace fd;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load("fdTest_" + QLocale::system().name());
    a.installTranslator(&appTranslator);

    // load configurations (device setup and test patterns)
    QString configPath = QString(getenv("USERPROFILE"));
    QJsonObject configOptions;
    QJsonObject testPatterns;
    QStringList configFileList;
    QFile configFile;

    configFileList << configOptionsFileName << testPatternsFileName;

    foreach (QString name, configFileList)
    {
        configFile.setFileName(QDir::toNativeSeparators(configPath + name));

        if(!configFile.exists())
        {
            QFile defaultFile(":/conf" + name);

            if (!defaultFile.copy(configFile.fileName()))
            {
                qWarning() << "cannot copy" << defaultFile.fileName() << "to" << configFile.fileName();
                exit(1);
            }
        }

        if (!configFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "cannot open" << configFile.fileName();
            exit(2);
        }

        QByteArray data = configFile.readAll();
        configFile.close();

        QJsonParseError parser;
        QJsonDocument setupDoc = QJsonDocument::fromJson(data, &parser);

        if (parser.error)
        {
            qWarning() << "json parse error in" << parser.errorString();
            exit(3);
        }

        if (setupDoc.isObject())
        {
            if (setupDoc.object().isEmpty())
            {
                qWarning() << "configuration is not defined in:" << configFile.fileName();
                exit(4);
            }
            else
            {
                if (name == configOptionsFileName)
                {
                    configOptions = setupDoc.object(); // configuration for a DUT
                }
                else if (name == testPatternsFileName)
                {
                    testPatterns = setupDoc.object(); // test patterns for a DUT
                }
            }
        }
    }

    MainWindow mainWindow(configOptions, testPatterns);
    mainWindow.show();

    return a.exec();
}
