#include "setupwizard.h"
#include "fd.h"
#include <QDebug>
#include <QByteArray>
#include <QWizardPage>
#include <QJsonObject>
#include <QJsonArray>
#include <QHBoxLayout>
#include <QSerialPortInfo>

using namespace fd;

SetupWizard::SetupWizard(QJsonObject &config, QWidget *parent) :
    QWizard(parent)
{
    if (!config.isEmpty())
    {
        if (!config.contains(DeviceSection) || !config[DeviceSection].isArray() ||
                config[DeviceSection].toArray().isEmpty())
        {
            qDebug() << "device type is not defined!";
        }

        if (!config.contains(FirmwareSection) || !config[FirmwareSection].isArray() ||
                config[FirmwareSection].toArray().isEmpty())
        {
            qDebug() << "firmware version is not defined!";
        }

        if (!config.contains(FirmwareSection) || !config[FirmwareSection].isArray() ||
                config[FirmwareSection].toArray().isEmpty())
        {
            qDebug() << "device interface is not defined!";
        }

        addPage(new FlurdisplayPage(config));
        addPage(new HostInterfacePage);
        addPage(new ConclusionPage);

        setWindowTitle(tr("Configuration"));

        mConfigOptions = config;
    }
}

SetupWizard::~SetupWizard()
{

}

FlurdisplayPage::FlurdisplayPage(QJsonObject &config, QWidget *parent) :
    QWizardPage(parent)
{
    setTitle(tr("Flurdisplay"));
    setSubTitle(tr("Select the type, firmware and interface of Flurdisplay"));

    // device type
    devTypeLabel = new QLabel(tr("&Type:"));
    devTypeCombo = new QComboBox;
    devTypeLabel->setBuddy(devTypeCombo);

    QJsonArray devices = config[DeviceSection].toArray();
    QJsonArray::Iterator i;
    int idx = 0;

    for(i = devices.begin(); i != devices.end(); ++i, ++idx)
    {
        if ((*i).isObject())
        {
            if ((*i).toObject().contains(ConfigName) && (*i).toObject()[ConfigName].isString() &&
                    (*i).toObject().contains(DevMaxChar) && (*i).toObject()[DevMaxChar].isDouble())
            {
                devTypeCombo->insertItem(idx, (*i).toObject()[ConfigName].toString("undefined"), (*i).toObject()[DevMaxChar].toInt());
            }
        }
    }

    registerField("devTypeName", devTypeCombo, "currentText", SIGNAL(currentIndexChanged(QString)));
    registerField("devTypeMaxChar", devTypeCombo, "currentData");

    // device firmware
    devFirmwareLabel = new QLabel(tr("&Firmware:"));
    devFirmwareCombo = new QComboBox;
    devFirmwareLabel->setBuddy(devFirmwareCombo);

    QJsonArray versions = config[FirmwareSection].toArray();

    for(i = versions.begin(); i != versions.end(); ++i, ++idx)
    {
        if ((*i).isObject())
        {
            if ((*i).toObject().contains(ConfigName) && (*i).toObject()[ConfigName].isString() &&
                    (*i).toObject().contains("major") && (*i).toObject()["major"].isDouble())
            {
                devFirmwareCombo->insertItem(idx, (*i).toObject()[ConfigName].toString("undefined"), (*i).toObject()["major"].toInt());
            }
        }
    }

    registerField("devFirmwareName", devFirmwareCombo, "currentText", SIGNAL(currentIndexChanged(QString)));
    registerField("devFirmwareMajor", devFirmwareCombo, "currentData");

    // device interface
    devInterfaceLabel = new QLabel(tr("&Interface:"));
    devInterfaceCombo = new QComboBox;
    devInterfaceLabel->setBuddy(devInterfaceCombo);

    QJsonArray interfaces = config[DevInterfaceSection].toArray();

    for(i = interfaces.begin(); i != interfaces.end(); ++i, ++idx)
    {
        if ((*i).isObject())
        {
            if ((*i).toObject().contains(ConfigName) && (*i).toObject()[ConfigName].isString() &&
                    (*i).toObject().contains(ConfigParam) && (*i).toObject()[ConfigParam].isString())
            {
                devInterfaceCombo->insertItem(idx, (*i).toObject()[ConfigName].toString("undefined"), (*i).toObject()[ConfigParam].toString());
            }
        }
    }

    registerField("devInterfaceName", devInterfaceCombo, "currentText", SIGNAL(currentIndexChanged(QString)));
    registerField("devInterfaceParam", devInterfaceCombo, "currentData");

    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget(devTypeLabel);
    layout->addWidget(devTypeCombo);

    layout->addWidget(devFirmwareLabel);
    layout->addWidget(devFirmwareCombo);

    layout->addWidget(devInterfaceLabel);
    layout->addWidget(devInterfaceCombo);

    setLayout(layout);
}

HostInterfacePage::HostInterfacePage(QWidget *parent) :
    QWizardPage(parent)
{
    setTitle(tr("Host interface"));
    setSubTitle(tr("Select the host interface"));

    hostInterfaceLabel = new QLabel(tr("&Interface:"));
    hostInterfaceCombo = new QComboBox;
    hostInterfaceLabel->setBuddy(hostInterfaceCombo);

    int idx = 0;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        hostInterfaceCombo->insertItem(idx, info.portName(), info.serialNumber());
    }

    registerField("hostInterfaceName", hostInterfaceCombo, "currentText", SIGNAL(currentIndexChanged(QString)));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(hostInterfaceLabel);
    layout->addWidget(hostInterfaceCombo);
    setLayout(layout);
}

ConclusionPage::ConclusionPage(QWidget *parent) :
    QWizardPage(parent)
{
    setTitle(tr("Conclusion"));
    setSubTitle(tr("The following settings are going to be applied!"));

    devTypeLabel = new QLabel;
    devFirmwareLabel = new QLabel;
    devInterfaceLabel = new QLabel;
    hostInterfaceLabel = new QLabel;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(devTypeLabel);
    layout->addWidget(devFirmwareLabel);
    layout->addWidget(devInterfaceLabel);
    layout->addWidget(hostInterfaceLabel);
    setLayout(layout);
}

void SetupWizard::accept()
{

    qDebug() << "devTypeName" << "=" << field("devTypeName").toString() <<
                "devTypeMaxChar" << "=" << QString::number(field("devTypeMaxChar").toInt()) <<

                "devFirmwareName" << "=" << field("devFirmwareName").toString() <<
                "devFirmwareMajor" << "=" << QString::number(field("devFirmwareMajor").toInt()) <<

                "devInterfaceName" << "=" << field("devInterfaceName").toString() <<
                "devInterfaceParam" << "=" << field("devInterfaceParam").toString() <<

                "hostInterfaceName" << "=" << field("hostInterfaceName").toString();

    QJsonObject configDevice;
    // device type
    configDevice[ConfigName] = field("devTypeName").toString();
    configDevice[DevMaxChar] = field("devTypeMaxChar").toInt();

    QJsonArray hiddenOption = mConfigOptions[DeviceSection].toArray();
    QJsonArray::Iterator i;

    for(i = hiddenOption.begin(); i != hiddenOption.end(); ++i)
    {
        if ((*i).isObject())
        {
            if ((*i).toObject().contains(ConfigName) && (*i).toObject()[ConfigName].isString())
            {
                if ((*i).toObject()[ConfigName].toString() == configDevice[ConfigName].toString())
                    configDevice[DevOnLongText] = (*i).toObject()[DevOnLongText].toString();  // "onLongText"
            }
        }
    }

    mConfigDev[DevSection] = configDevice;

    // device firmware
    QJsonObject configFirmware;
    configFirmware[ConfigName] = field("devFirmwareName").toString();
    configFirmware["major"] = field("devFirmwareMajor").toInt();
    mConfigDev[FirmwareSection] = configFirmware;

    // device interface
    QJsonObject configDevInterface;
    configDevInterface[ConfigName] = field("devInterfaceName").toString();
    configDevInterface[ConfigParam] = field("devInterfaceParam").toString();

    hiddenOption = mConfigOptions[DevInterfaceSection].toArray();

    for(i = hiddenOption.begin(); i != hiddenOption.end(); ++i)
    {
        if ((*i).isObject())
        {
            if ((*i).toObject().contains(ConfigName) && (*i).toObject()[ConfigName].isString())
            {
                if ((*i).toObject()[ConfigName].toString() == configDevInterface[ConfigName].toString())
                    configDevInterface[DevInterfaceCmd] = (*i).toObject()[DevInterfaceCmd].toString();  // "command"
            }
        }
    }

    mConfigDev[DevInterfaceSection] = configDevInterface;

    // host interface
    QJsonObject configHostInterface;
    configHostInterface[ConfigName] = field("hostInterfaceName").toString();
    configHostInterface[ConfigParam] = field("devInterfaceParam").toString();
    mConfigDev[HostInterfaceSection] = configHostInterface;

    QDialog::accept();
}

void ConclusionPage::initializePage()
{
    QString text(tr("Device type"));
    text.append(": ");
    text.append(field("devTypeName").toString());
    devTypeLabel->setText(text);

    text = tr("Device firmware");
    text.append(": ");
    text.append(field("devFirmwareName").toString());
    devFirmwareLabel->setText(text);

    text = tr("Device interface");
    text.append(": ");
    text.append(field("devInterfaceName").toString());
    devInterfaceLabel->setText(text);

    text = tr("Host interface");
    text.append(": ");
    text.append(field("hostInterfaceName").toString());
    hostInterfaceLabel->setText(text);
}

QJsonObject SetupWizard::configDev()
{
    return mConfigDev;
}
