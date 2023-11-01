// Copyright (C) 2019 Kovalev Dmitry <kovalevda1991@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jlinkgdbserverprovider.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaltr.h>
#include <baremetal/debugserverprovidermanager.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QLabel>

using namespace Utils;

namespace BareMetal::Internal {

const char executableFileKeyC[] = "ExecutableFile";
const char jlinkDeviceKeyC[] = "JLinkDevice";
const char jlinkHostInterfaceKeyC[] = "JLinkHostInterface";
const char jlinkHostInterfaceIPAddressKeyC[] = "JLinkHostInterfaceIPAddress";
const char jlinkTargetInterfaceKeyC[] = "JLinkTargetInterface";
const char jlinkTargetInterfaceSpeedKeyC[] = "JLinkTargetInterfaceSpeed";
const char additionalArgumentsKeyC[] = "AdditionalArguments";

// JLinkGdbServerProviderConfigWidget

class JLinkGdbServerProvider;

class JLinkGdbServerProviderConfigWidget final : public GdbServerProviderConfigWidget
{
public:
    explicit JLinkGdbServerProviderConfigWidget(JLinkGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    void populateHostInterfaces();
    void populateTargetInterfaces();
    void populateTargetSpeeds();

    void setHostInterface(const QString &newIface);
    void setTargetInterface(const QString &newIface);
    void setTargetSpeed(const QString &newSpeed);

    void updateAllowedControls();

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    PathChooser *m_executableFileChooser = nullptr;

    QWidget *m_hostInterfaceWidget = nullptr;
    QComboBox *m_hostInterfaceComboBox = nullptr;
    QLabel *m_hostInterfaceAddressLabel = nullptr;
    QLineEdit *m_hostInterfaceAddressLineEdit = nullptr;

    QWidget *m_targetInterfaceWidget = nullptr;
    QComboBox *m_targetInterfaceComboBox = nullptr;
    QLabel *m_targetInterfaceSpeedLabel = nullptr;
    QComboBox *m_targetInterfaceSpeedComboBox = nullptr;

    QLineEdit *m_jlinkDeviceLineEdit = nullptr;
    QPlainTextEdit *m_additionalArgumentsTextEdit = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

// JLinkGdbServerProvider

class JLinkGdbServerProvider final : public GdbServerProvider
{
public:
    void toMap(Store &data) const final;
    void fromMap(const Store &data) final;

    bool operator==(const IDebugServerProvider &other) const final;

    QString channelString() const final;
    CommandLine command() const final;

    QSet<StartupMode> supportedStartupModes() const final;
    bool isValid() const final;

private:
    JLinkGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    FilePath m_executableFile;
    QString m_jlinkDevice;
    QString m_jlinkHost = {"USB"};
    QString m_jlinkHostAddr;
    QString m_jlinkTargetIface = {"SWD"};
    QString m_jlinkTargetIfaceSpeed = {"12000"};
    QString m_additionalArguments;

    friend class JLinkGdbServerProviderConfigWidget;
    friend class JLinkGdbServerProviderFactory;
};

JLinkGdbServerProvider::JLinkGdbServerProvider()
    : GdbServerProvider(Constants::GDBSERVER_JLINK_PROVIDER_ID)
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setChannel("localhost", 2331);
    setTypeDisplayName(Tr::tr("JLink"));
    setConfigurationWidgetCreator([this] { return new JLinkGdbServerProviderConfigWidget(this); });
}

QString JLinkGdbServerProvider::defaultInitCommands()
{
    return {"set remote hardware-breakpoint-limit 6\n"
        "set remote hardware-watchpoint-limit 4\n"
        "monitor reset halt\n"
        "load\n"
        "monitor reset halt\n"};
}

QString JLinkGdbServerProvider::defaultResetCommands()
{
    return {"monitor reset halt\n"};
}

QString JLinkGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    default: // wrong
        return {};
    }
}

CommandLine JLinkGdbServerProvider::command() const
{
    CommandLine cmd{m_executableFile};

    if (startupMode() == StartupOnNetwork)
        cmd.addArgs("-port " + QString::number(channel().port()), CommandLine::Raw);

    if (m_jlinkHost == "USB") {
        cmd.addArgs("-select usb", CommandLine::Raw);
    } else if (m_jlinkHost == "IP") {
        cmd.addArgs("-select ip=" + m_jlinkHostAddr, CommandLine::Raw);
    }

    if (!m_jlinkTargetIface.isEmpty()) {
        cmd.addArgs("-if " + m_jlinkTargetIface, CommandLine::Raw);
        if (!m_jlinkTargetIfaceSpeed.isEmpty())
            cmd.addArgs("-speed " + m_jlinkTargetIfaceSpeed, CommandLine::Raw);
    }

    if (!m_jlinkDevice.isEmpty())
        cmd.addArgs("-device " + m_jlinkDevice, CommandLine::Raw);

    if (!m_additionalArguments.isEmpty())
        cmd.addArgs(m_additionalArguments, CommandLine::Raw);

    return cmd;
}

QSet<GdbServerProvider::StartupMode>
JLinkGdbServerProvider::supportedStartupModes() const
{
    return {StartupOnNetwork};
}

bool JLinkGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    const StartupMode m = startupMode();

    if (m == StartupOnNetwork) {
        if (channel().host().isEmpty())
            return false;
    }

    return true;
}

void JLinkGdbServerProvider::toMap(Store &data) const
{
    GdbServerProvider::toMap(data);
    data.insert(executableFileKeyC, m_executableFile.toSettings());
    data.insert(jlinkDeviceKeyC, m_jlinkDevice);
    data.insert(jlinkHostInterfaceKeyC, m_jlinkHost);
    data.insert(jlinkHostInterfaceIPAddressKeyC, m_jlinkHostAddr);
    data.insert(jlinkTargetInterfaceKeyC, m_jlinkTargetIface);
    data.insert(jlinkTargetInterfaceSpeedKeyC, m_jlinkTargetIfaceSpeed);
    data.insert(additionalArgumentsKeyC, m_additionalArguments);
}

void JLinkGdbServerProvider::fromMap(const Store &data)
{
    GdbServerProvider::fromMap(data);
    m_executableFile = FilePath::fromSettings(data.value(executableFileKeyC));
    m_jlinkDevice = data.value(jlinkDeviceKeyC).toString();
    m_additionalArguments = data.value(additionalArgumentsKeyC).toString();
    m_jlinkHost = data.value(jlinkHostInterfaceKeyC).toString();
    m_jlinkHostAddr = data.value(jlinkHostInterfaceIPAddressKeyC).toString();
    m_jlinkTargetIface = data.value(jlinkTargetInterfaceKeyC).toString();
    m_jlinkTargetIfaceSpeed = data.value(jlinkTargetInterfaceSpeedKeyC).toString();
}

bool JLinkGdbServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const JLinkGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_jlinkDevice == p->m_jlinkDevice
            && m_jlinkHost == p->m_jlinkHost
            && m_jlinkHostAddr == p->m_jlinkHostAddr
            && m_jlinkTargetIface == p->m_jlinkTargetIface
            && m_jlinkTargetIfaceSpeed == p->m_jlinkTargetIfaceSpeed
            && m_additionalArguments == p->m_additionalArguments;
}

// JLinkGdbServerProviderFactory

JLinkGdbServerProviderFactory::JLinkGdbServerProviderFactory()
{
    setId(Constants::GDBSERVER_JLINK_PROVIDER_ID);
    setDisplayName(Tr::tr("JLink"));
    setCreator([] { return new JLinkGdbServerProvider; });
}

// JLinkGdbServerProviderConfigWidget

JLinkGdbServerProviderConfigWidget::JLinkGdbServerProviderConfigWidget(
        JLinkGdbServerProvider *provider)
    : GdbServerProviderConfigWidget(provider)
{
    Q_ASSERT(provider);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(Tr::tr("Host:"), m_hostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executableFileChooser->setCommandVersionArguments({"--version"});
    if (HostOsInfo::hostOs() == OsTypeWindows) {
        m_executableFileChooser->setPromptDialogFilter(Tr::tr("JLink GDB Server (JLinkGDBServerCL.exe)"));
        m_executableFileChooser->lineEdit()->setPlaceholderText("JLinkGDBServerCL.exe");
    } else {
        m_executableFileChooser->setPromptDialogFilter(Tr::tr("JLink GDB Server (JLinkGDBServer)"));
        m_executableFileChooser->lineEdit()->setPlaceholderText("JLinkGDBServer");
    }
    m_mainLayout->addRow(Tr::tr("Executable file:"), m_executableFileChooser);

    // Host interface settings.
    m_hostInterfaceWidget = new QWidget(this);
    m_hostInterfaceComboBox = new QComboBox(m_hostInterfaceWidget);
    m_hostInterfaceAddressLabel = new QLabel(m_hostInterfaceWidget);
    m_hostInterfaceAddressLabel->setText(Tr::tr("IP Address"));
    m_hostInterfaceAddressLineEdit = new QLineEdit(m_hostInterfaceWidget);
    const auto hostInterfaceLayout = new QHBoxLayout(m_hostInterfaceWidget);
    hostInterfaceLayout->setContentsMargins(0, 0, 0, 0);
    hostInterfaceLayout->addWidget(m_hostInterfaceComboBox);
    hostInterfaceLayout->addWidget(m_hostInterfaceAddressLabel);
    hostInterfaceLayout->addWidget(m_hostInterfaceAddressLineEdit);
    m_mainLayout->addRow(Tr::tr("Host interface:"), m_hostInterfaceWidget);

    // Target interface settings.
    m_targetInterfaceWidget = new QWidget(this);
    m_targetInterfaceComboBox = new QComboBox(m_targetInterfaceWidget);
    m_targetInterfaceSpeedLabel = new QLabel(m_targetInterfaceWidget);
    m_targetInterfaceSpeedLabel->setText(Tr::tr("Speed"));
    m_targetInterfaceSpeedComboBox = new QComboBox(m_targetInterfaceWidget);
    const auto targetInterfaceLayout = new QHBoxLayout(m_targetInterfaceWidget);
    targetInterfaceLayout->setContentsMargins(0, 0, 0, 0);
    targetInterfaceLayout->addWidget(m_targetInterfaceComboBox);
    targetInterfaceLayout->addWidget(m_targetInterfaceSpeedLabel);
    targetInterfaceLayout->addWidget(m_targetInterfaceSpeedComboBox);
    m_mainLayout->addRow(Tr::tr("Target interface:"), m_targetInterfaceWidget);

    m_jlinkDeviceLineEdit = new QLineEdit(this);
    m_mainLayout->addRow(Tr::tr("Device:"), m_jlinkDeviceLineEdit);

    m_additionalArgumentsTextEdit = new QPlainTextEdit(this);
    m_mainLayout->addRow(Tr::tr("Additional arguments:"), m_additionalArgumentsTextEdit);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(Tr::tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(Tr::tr("Reset commands:"), m_resetCommandsTextEdit);

    populateHostInterfaces();
    populateTargetInterfaces();
    populateTargetSpeeds();
    addErrorLabel();
    setFromProvider();

    const auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit);
    chooser->addSupportedWidget(m_resetCommandsTextEdit);

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_executableFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_jlinkDeviceLineEdit, &QLineEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_additionalArgumentsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_initCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_hostInterfaceComboBox, &QComboBox::currentTextChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_hostInterfaceAddressLineEdit, &QLineEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_targetInterfaceComboBox, &QComboBox::currentTextChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_targetInterfaceSpeedComboBox, &QComboBox::currentTextChanged,
            this, &GdbServerProviderConfigWidget::dirty);

    connect(m_hostInterfaceComboBox, &QComboBox::currentIndexChanged,
            this, &JLinkGdbServerProviderConfigWidget::updateAllowedControls);
    connect(m_targetInterfaceComboBox, &QComboBox::currentIndexChanged,
            this, &JLinkGdbServerProviderConfigWidget::updateAllowedControls);
    connect(m_targetInterfaceSpeedComboBox, &QComboBox::currentIndexChanged,
            this, &JLinkGdbServerProviderConfigWidget::updateAllowedControls);
}

void JLinkGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<JLinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->m_executableFile = m_executableFileChooser->filePath();
    p->m_jlinkDevice = m_jlinkDeviceLineEdit->text();
    p->m_jlinkHost = m_hostInterfaceComboBox->currentData().toString();
    p->m_jlinkHostAddr = m_hostInterfaceAddressLineEdit->text();
    p->m_jlinkTargetIface = m_targetInterfaceComboBox->currentData().toString();
    p->m_jlinkTargetIfaceSpeed = m_targetInterfaceSpeedComboBox->currentData().toString();
    p->m_additionalArguments = m_additionalArgumentsTextEdit->toPlainText();
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
    GdbServerProviderConfigWidget::apply();
}

void JLinkGdbServerProviderConfigWidget::discard()
{
    setFromProvider();
    GdbServerProviderConfigWidget::discard();
}

void JLinkGdbServerProviderConfigWidget::populateHostInterfaces()
{
    m_hostInterfaceComboBox->addItem(Tr::tr("Default"));
    m_hostInterfaceComboBox->addItem(Tr::tr("USB"), "USB");
    m_hostInterfaceComboBox->addItem(Tr::tr("TCP/IP"), "IP");
}

void JLinkGdbServerProviderConfigWidget::populateTargetInterfaces()
{
    m_targetInterfaceComboBox->addItem(Tr::tr("Default"));
    m_targetInterfaceComboBox->addItem(Tr::tr("JTAG"), "JTAG");
    m_targetInterfaceComboBox->addItem(Tr::tr("Compact JTAG"), "cJTAG");
    m_targetInterfaceComboBox->addItem(Tr::tr("SWD"), "SWD");
    m_targetInterfaceComboBox->addItem(Tr::tr("Renesas RX FINE"), "FINE");
    m_targetInterfaceComboBox->addItem(Tr::tr("ICSP"), "ICSP");
}

void JLinkGdbServerProviderConfigWidget::populateTargetSpeeds()
{
    m_targetInterfaceSpeedComboBox->addItem(Tr::tr("Default"));
    m_targetInterfaceSpeedComboBox->addItem(Tr::tr("Auto"), "auto");
    m_targetInterfaceSpeedComboBox->addItem(Tr::tr("Adaptive"), "adaptive");

    const QStringList fixedSpeeds = {"1", "5", "10", "20", "30", "50", "100", "200", "300",
                                     "400", "500", "600", "750", "800", "900", "1000", "1334",
                                     "1600", "2000",  "2667" ,"3200", "4000", "4800", "5334",
                                     "6000", "8000", "9600", "12000", "15000", "20000", "25000",
                                     "30000", "40000", "50000"};
    for (const auto &fixedSpeed : fixedSpeeds)
        m_targetInterfaceSpeedComboBox->addItem(Tr::tr("%1 kHz").arg(fixedSpeed), fixedSpeed);
}

void JLinkGdbServerProviderConfigWidget::setHostInterface(const QString &newIface)
{
    for (int index = 0; index < m_hostInterfaceComboBox->count(); ++index) {
        const auto iface = m_hostInterfaceComboBox->itemData(index).toString();
        if (iface == newIface) {
            m_hostInterfaceComboBox->setCurrentIndex(index);
            return;
        }
    }
    // Falling back to the first default entry.
    m_hostInterfaceComboBox->setCurrentIndex(0);
}

void JLinkGdbServerProviderConfigWidget::setTargetInterface(const QString &newIface)
{
    for (int index = 0; index < m_targetInterfaceComboBox->count(); ++index) {
        const auto iface = m_targetInterfaceComboBox->itemData(index).toString();
        if (iface == newIface) {
            m_targetInterfaceComboBox->setCurrentIndex(index);
            return;
        }
    }
    // Falling back to the first default entry.
    m_targetInterfaceComboBox->setCurrentIndex(0);
}

void JLinkGdbServerProviderConfigWidget::setTargetSpeed(const QString &newSpeed)
{
    for (int index = 0; index < m_targetInterfaceSpeedComboBox->count(); ++index) {
        const auto speed = m_targetInterfaceSpeedComboBox->itemData(index).toString();
        if (speed == newSpeed) {
            m_targetInterfaceSpeedComboBox->setCurrentIndex(index);
            return;
        }
    }
    // Falling back to the first default entry.
    m_targetInterfaceSpeedComboBox->setCurrentIndex(0);
}

void JLinkGdbServerProviderConfigWidget::updateAllowedControls()
{
    const bool isHostIfaceIPSelected = (m_hostInterfaceComboBox->currentData().toString() == "IP");
    m_hostInterfaceAddressLabel->setVisible(isHostIfaceIPSelected);
    m_hostInterfaceAddressLineEdit->setVisible(isHostIfaceIPSelected);

    const bool isTargetIfaceDefaultSelected = !m_targetInterfaceComboBox->currentData().isValid();
    m_targetInterfaceSpeedLabel->setVisible(!isTargetIfaceDefaultSelected);
    m_targetInterfaceSpeedComboBox->setVisible(!isTargetIfaceDefaultSelected);
}

void JLinkGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<JLinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    m_additionalArgumentsTextEdit->setPlainText(p->m_additionalArguments);
    m_executableFileChooser->setFilePath(p->m_executableFile);
    m_hostInterfaceAddressLineEdit->setText(p->m_jlinkHostAddr);
    m_hostWidget->setChannel(p->channel());
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_jlinkDeviceLineEdit->setText(p->m_jlinkDevice);
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());

    setHostInterface(p->m_jlinkHost);
    setTargetInterface(p->m_jlinkTargetIface);
    setTargetSpeed(p->m_jlinkTargetIfaceSpeed);

    updateAllowedControls();
}

} // BareMetal::Internal
