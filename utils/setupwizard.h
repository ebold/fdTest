#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H
#include <QWizard>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QJsonObject>

class SetupWizard : public QWizard
{
    Q_OBJECT
public:
    SetupWizard(QJsonObject &config, QWidget *parent = 0);
    ~SetupWizard();

    void accept()   Q_DECL_OVERRIDE;    // this slot is reimplemented

    QJsonObject configDev();    // command is not included in device interface

private:
    QJsonObject mConfigDev;
    QJsonObject mConfigOptions;
};

class FlurdisplayPage : public QWizardPage
{
    Q_OBJECT
public:
    FlurdisplayPage(QJsonObject &config, QWidget *parent = 0);

private:
    QLabel *devTypeLabel;
    QComboBox *devTypeCombo;

    QLabel *devFirmwareLabel;
    QComboBox *devFirmwareCombo;

    QLabel *devInterfaceLabel;
    QComboBox *devInterfaceCombo;
};

class HostInterfacePage : public QWizardPage
{
    Q_OBJECT
public:
    HostInterfacePage(QWidget *parent = 0);

private:
    QLabel *hostInterfaceLabel;
    QComboBox *hostInterfaceCombo;
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT
public:
    ConclusionPage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QLabel *devTypeLabel;
    QLabel *devFirmwareLabel;
    QLabel *devInterfaceLabel;
    QLabel *hostInterfaceLabel;
};


#endif // SETUPWIZARD_H
