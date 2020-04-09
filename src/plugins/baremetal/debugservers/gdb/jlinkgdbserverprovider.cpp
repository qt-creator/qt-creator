/****************************************************************************
**
** Copyright (C) 2019 Kovalev Dmitry <kovalevda1991@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "jlinkgdbserverprovider.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/debugserverprovidermanager.h>

#include <coreplugin/variablechooser.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QLabel>

using namespace Utils;

namespace BareMetal {
namespace Internal {

const char executableFileKeyC[] = "BareMetal.JLinkGdbServerProvider.ExecutableFile";
const char jlinkDeviceKeyC[] = "BareMetal.JLinkGdbServerProvider.JLinkDevice";
const char jlinkHostInterfaceKeyC[] = "BareMetal.JLinkGdbServerProvider.JLinkHostInterface";
const char jlinkHostInterfaceIPAddressKeyC[] = "BareMetal.JLinkGdbServerProvider.JLinkHostInterfaceIPAddress";
const char jlinkTargetInterfaceKeyC[] = "BareMetal.JLinkGdbServerProvider.JLinkTargetInterface";
const char jlinkTargetInterfaceSpeedKeyC[] = "BareMetal.JLinkGdbServerProvider.JLinkTargetInterfaceSpeed";
const char additionalArgumentsKeyC[] = "BareMetal.JLinkGdbServerProvider.AdditionalArguments";

// JLinkGdbServerProvider

JLinkGdbServerProvider::JLinkGdbServerProvider()
    : GdbServerProvider(Constants::GDBSERVER_JLINK_PROVIDER_ID)
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setChannel("localhost", 2331);
    setSettingsKeyBase("BareMetal.JLinkGdbServerProvider");
    setTypeDisplayName(GdbServerProvider::tr("JLink"));
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
        cmd.addArgs("-select usb ", CommandLine::Raw);
    } else
        if (m_jlinkHost == "IP") {
            cmd.addArgs("-select ip=" + m_jlinkHostAddr + " ", CommandLine::Raw);
        }

    if (m_jlinkTargetIface != "Default") {
        cmd.addArgs("-if " + m_jlinkTargetIface +
                    " -speed " + m_jlinkTargetIfaceSpeed + " ", CommandLine::Raw);
    }

    if (!m_jlinkDevice.isEmpty())
        cmd.addArgs("-device " + m_jlinkDevice + " ", CommandLine::Raw);

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

QVariantMap JLinkGdbServerProvider::toMap() const
{
    QVariantMap data = GdbServerProvider::toMap();
    data.insert(executableFileKeyC, m_executableFile.toVariant());
    data.insert(jlinkDeviceKeyC, m_jlinkDevice);
    data.insert(jlinkHostInterfaceKeyC, m_jlinkHost);
    data.insert(jlinkHostInterfaceIPAddressKeyC, m_jlinkHostAddr);
    data.insert(jlinkTargetInterfaceKeyC, m_jlinkTargetIface);
    data.insert(jlinkTargetInterfaceSpeedKeyC, m_jlinkTargetIfaceSpeed);
    data.insert(additionalArgumentsKeyC, m_additionalArguments);
    return data;
}

bool JLinkGdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!GdbServerProvider::fromMap(data))
        return false;

    m_executableFile = FilePath::fromVariant(data.value(executableFileKeyC));
    m_jlinkDevice = data.value(jlinkDeviceKeyC).toString();
    m_additionalArguments = data.value(additionalArgumentsKeyC).toString();
    m_jlinkHost = data.value(jlinkHostInterfaceKeyC).toString();
    m_jlinkHostAddr = data.value(jlinkHostInterfaceIPAddressKeyC).toString();
    m_jlinkTargetIface = data.value(jlinkTargetInterfaceKeyC).toString();
    m_jlinkTargetIfaceSpeed = data.value(jlinkTargetInterfaceSpeedKeyC).toString();
    return true;
}

bool JLinkGdbServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const JLinkGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_additionalArguments == p->m_additionalArguments;
}

// JLinkGdbServerProviderFactory

JLinkGdbServerProviderFactory::JLinkGdbServerProviderFactory()
{
    setId(Constants::GDBSERVER_JLINK_PROVIDER_ID);
    setDisplayName(GdbServerProvider::tr("JLink"));
    setCreator([] { return new JLinkGdbServerProvider; });
}

// JLinkGdbServerProviderConfigWidget

JLinkGdbServerProviderConfigWidget::JLinkGdbServerProviderConfigWidget(
        JLinkGdbServerProvider *provider)
    : GdbServerProviderConfigWidget(provider)
{
    Q_ASSERT(provider);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executableFileChooser->setCommandVersionArguments({"--version"});
    if (HostOsInfo::hostOs() == OsTypeWindows) {
        m_executableFileChooser->setPromptDialogFilter(tr("JLink GDB Server (JLinkGDBServerCL.exe)"));
        m_executableFileChooser->lineEdit()->setPlaceholderText("JLinkGDBServerCL.exe");
    } else {
        m_executableFileChooser->setPromptDialogFilter(tr("JLink GDB Server (JLinkGDBServer)"));
        m_executableFileChooser->lineEdit()->setPlaceholderText("JLinkGDBServer");
    }
    m_mainLayout->addRow(tr("Executable file:"), m_executableFileChooser);

    //Host interface settings
    m_hostInterfaceWidget = new QWidget(this);
    m_hostInterfaceComboBox = new QComboBox(m_hostInterfaceWidget);
    m_hostInterfaceComboBox->insertItem(HostInterfaceDefault, tr("Default"));
    m_hostInterfaceComboBox->insertItem(HostInterfaceUSB, "USB");
    m_hostInterfaceComboBox->insertItem(HostInterfaceIP, "IP");

    m_hostInterfaceAddressLabel = new QLabel(m_hostInterfaceWidget);
    m_hostInterfaceAddressLabel->setText(tr("IP Address"));
    m_hostInterfaceAddressLineEdit = new QLineEdit(m_hostInterfaceWidget);

    const auto hostInterfaceLayout = new QHBoxLayout(m_hostInterfaceWidget);
    hostInterfaceLayout->setContentsMargins(0, 0, 0, 0);
    hostInterfaceLayout->addWidget(m_hostInterfaceComboBox);
    hostInterfaceLayout->addWidget(m_hostInterfaceAddressLabel);
    hostInterfaceLayout->addWidget(m_hostInterfaceAddressLineEdit);
    m_mainLayout->addRow(tr("Host interface:"), m_hostInterfaceWidget);

    //Target interface settings
    m_targetInterfaceWidget = new QWidget(this);
    m_targetInterfaceComboBox = new QComboBox(m_targetInterfaceWidget);
    m_targetInterfaceComboBox->insertItem(TargetInterfaceDefault, tr("Default"));
    m_targetInterfaceComboBox->insertItem(TargetInterfaceJTAG, "JTAG");
    m_targetInterfaceComboBox->insertItem(TargetInterfaceSWD, "SWD");
    m_targetInterfaceComboBox->insertItem(TargetInterfaceFINE, "FINE");
    m_targetInterfaceComboBox->insertItem(TargetInterface2Wire, "2-wire-JTAG-PIC32");

    m_targetInterfaceSpeedLabel = new QLabel(m_targetInterfaceWidget);
    m_targetInterfaceSpeedLabel->setText(tr("Speed"));

    m_targetInterfaceSpeedComboBox = new QComboBox(m_targetInterfaceWidget);
    m_targetInterfaceSpeedComboBox->insertItems(0, m_targetSpeedList);

    m_targetInterfaceSpeedUnitsLabel = new QLabel(m_targetInterfaceWidget);
    m_targetInterfaceSpeedUnitsLabel->setText(tr("kHz"));

    const auto targetInterfaceLayout = new QHBoxLayout(m_targetInterfaceWidget);
    targetInterfaceLayout->setContentsMargins(0, 0, 0, 0);
    targetInterfaceLayout->addWidget(m_targetInterfaceComboBox);
    targetInterfaceLayout->addWidget(m_targetInterfaceSpeedLabel);
    targetInterfaceLayout->addWidget(m_targetInterfaceSpeedComboBox);
    targetInterfaceLayout->addWidget(m_targetInterfaceSpeedUnitsLabel);

    m_mainLayout->addRow(tr("Target interface:"), m_targetInterfaceWidget);
    //

    m_jlinkDeviceLineEdit = new QLineEdit(this);
    m_mainLayout->addRow(tr("Device:"), m_jlinkDeviceLineEdit);

    m_additionalArgumentsTextEdit = new QPlainTextEdit(this);
    m_mainLayout->addRow(tr("Additional arguments:"), m_additionalArgumentsTextEdit);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);



    addErrorLabel();
    setFromProvider();

    const auto chooser = new Core::VariableChooser(this);
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

    connect(m_hostInterfaceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JLinkGdbServerProviderConfigWidget::hostInterfaceChanged);
    connect(m_targetInterfaceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JLinkGdbServerProviderConfigWidget::targetInterfaceChanged);
}

void JLinkGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<JLinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->m_executableFile = m_executableFileChooser->filePath();
    p->m_jlinkDevice = m_jlinkDeviceLineEdit->text();
    p->m_jlinkHost = m_hostInterfaceComboBox->currentText();
    p->m_jlinkHostAddr = m_hostInterfaceAddressLineEdit->text();
    p->m_jlinkTargetIface = m_targetInterfaceComboBox->currentText();
    p->m_jlinkTargetIfaceSpeed = m_targetInterfaceSpeedComboBox->currentText();
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

void JLinkGdbServerProviderConfigWidget::hostInterfaceChanged()
{
    const HostInterface selectedInterface = static_cast<HostInterface>(
                m_hostInterfaceComboBox->currentIndex());
    const bool isHostIfaceIP = selectedInterface == HostInterfaceIP;
    m_hostInterfaceAddressLabel->setVisible(isHostIfaceIP);
    m_hostInterfaceAddressLineEdit->setVisible(isHostIfaceIP);
}

void JLinkGdbServerProviderConfigWidget::targetInterfaceChanged()
{
    const TargetInterface selectedInterface = static_cast<TargetInterface>(
                m_targetInterfaceComboBox->currentIndex());
    const bool isDefault = selectedInterface == TargetInterfaceDefault;
    m_targetInterfaceSpeedLabel->setVisible(!isDefault);
    m_targetInterfaceSpeedComboBox->setVisible(!isDefault);
    m_targetInterfaceSpeedUnitsLabel->setVisible(!isDefault);
}

void JLinkGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<JLinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    m_hostWidget->setChannel(p->channel());
    m_executableFileChooser->setFilePath(p->m_executableFile);
    m_jlinkDeviceLineEdit->setText(p->m_jlinkDevice);
    m_additionalArgumentsTextEdit->setPlainText(p->m_additionalArguments);
    m_jlinkDeviceLineEdit->setText( p->m_jlinkDevice);
    m_hostInterfaceComboBox->setCurrentText(p->m_jlinkHost);
    m_hostInterfaceAddressLineEdit->setText(p->m_jlinkHostAddr);
    m_targetInterfaceComboBox->setCurrentText(p->m_jlinkTargetIface);
    m_targetInterfaceSpeedComboBox->setCurrentText(p->m_jlinkTargetIfaceSpeed);
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
    hostInterfaceChanged();
    targetInterfaceChanged();
}

} // namespace Internal
} // namespace BareMetal
