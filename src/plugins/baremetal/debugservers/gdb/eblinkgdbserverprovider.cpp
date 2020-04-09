/****************************************************************************
**
** Copyright (C) 2019 Andrey Sobol <andrey.sobol.nn@gmail.com>
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

#include "eblinkgdbserverprovider.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/debugserverprovidermanager.h>

#include <coreplugin/variablechooser.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QFileInfo>
#include <QDir>

using namespace Utils;

namespace BareMetal {
namespace Internal {

const char executableFileKeyC[] = "BareMetal.EBlinkGdbServerProvider.ExecutableFile";
const char verboseLevelKeyC[] = "BareMetal.EBlinkGdbServerProvider.VerboseLevel";
const char deviceScriptC[] = "BareMetal.EBlinkGdbServerProvider.DeviceScript";
const char interfaceTypeC[] = "BareMetal.EBlinkGdbServerProvider.InterfaceType";
const char interfaceResetOnConnectC[] = "BareMetal.EBlinkGdbServerProvider.interfaceResetOnConnect";
const char interfaceSpeedC[] = "BareMetal.EBlinkGdbServerProvider.InterfaceSpeed";
const char interfaceExplicidDeviceC[] = "BareMetal.EBlinkGdbServerProvider.InterfaceExplicidDevice";
const char targetNameC[] = "BareMetal.EBlinkGdbServerProvider.TargetName";
const char targetDisableStackC[] = "BareMetal.EBlinkGdbServerProvider.TargetDisableStack";
const char gdbShutDownAfterDisconnectC[] = "BareMetal.EBlinkGdbServerProvider.GdbShutDownAfterDisconnect";
const char gdbNotUseCacheC[] = "BareMetal.EBlinkGdbServerProvider.GdbNotUseCache";

// EBlinkGdbServerProvider

EBlinkGdbServerProvider::EBlinkGdbServerProvider()
    : GdbServerProvider(Constants::GDBSERVER_EBLINK_PROVIDER_ID)
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setChannel("127.0.0.1", 2331);
    setSettingsKeyBase("BareMetal.EBlinkGdbServerProvider");
    setTypeDisplayName(GdbServerProvider::tr("EBlink"));
    setConfigurationWidgetCreator([this] { return new EBlinkGdbServerProviderConfigWidget(this); });
}

QString EBlinkGdbServerProvider::defaultInitCommands()
{
    return {"monitor reset halt\n"
        "load\n"
        "monitor reset halt\n"
        "break main\n"};
}

QString EBlinkGdbServerProvider::defaultResetCommands()
{
    return {"monitor reset halt\n"};
}

QString EBlinkGdbServerProvider::scriptFileWoExt() const
{
    // Server starts only without extension in scriptname
    return m_deviceScript.toFileInfo().absolutePath() +
        QDir::separator() + m_deviceScript.toFileInfo().baseName();
}

QString EBlinkGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    default:
        return {};
    }
}

CommandLine EBlinkGdbServerProvider::command() const
{
    CommandLine cmd{m_executableFile, {}};
    QStringList interFaceTypeStrings = {"swd", "jtag"};

    // Obligatorily -I
    cmd.addArg("-I");
    QString interfaceArgs("stlink,%1,speed=%2");
    interfaceArgs = interfaceArgs.arg(interFaceTypeStrings.at(m_interfaceType))
                                .arg(QString::number(m_interfaceSpeed));
    if (!m_interfaceResetOnConnect)
        interfaceArgs.append(",dr");
    if (!m_interfaceExplicidDevice.trimmed().isEmpty())
        interfaceArgs.append(",device=" + m_interfaceExplicidDevice.trimmed());
    cmd.addArg(interfaceArgs);

    // Obligatorily -D
    cmd.addArg("-D");
    cmd.addArg(scriptFileWoExt());

    // Obligatorily -G
    cmd.addArg("-G");
    QString gdbArgs("port=%1,address=%2");
    gdbArgs = gdbArgs.arg(QString::number(channel().port()))
                    .arg(channel().host());
    if (m_gdbNotUseCache)
        gdbArgs.append(",nc");
    if (m_gdbShutDownAfterDisconnect)
        gdbArgs.append(",S");
    cmd.addArg(gdbArgs);

    cmd.addArg("-T");
    QString targetArgs(m_targetName.trimmed());
    if (m_targetDisableStack)
        targetArgs.append(",nu");
    cmd.addArg(targetArgs);

    cmd.addArg("-v");
    cmd.addArg(QString::number(m_verboseLevel));

    if (HostOsInfo::isWindowsHost())
        cmd.addArg("-g"); // no gui

    return cmd;
}

QSet<GdbServerProvider::StartupMode>
EBlinkGdbServerProvider::supportedStartupModes() const
{
    return {StartupOnNetwork};
}

bool EBlinkGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    switch (startupMode()) {
    case StartupOnNetwork:
        return !channel().host().isEmpty() && !m_executableFile.isEmpty()
                                           && !m_deviceScript.isEmpty();
    default:
        return false;
    }
}

QVariantMap EBlinkGdbServerProvider::toMap() const
{
    QVariantMap data = GdbServerProvider::toMap();
    data.insert(executableFileKeyC, m_executableFile.toVariant());
    data.insert(verboseLevelKeyC, m_verboseLevel);
    data.insert(interfaceTypeC, m_interfaceType);
    data.insert(deviceScriptC, m_deviceScript.toVariant());
    data.insert(interfaceResetOnConnectC, m_interfaceResetOnConnect);
    data.insert(interfaceSpeedC, m_interfaceSpeed);
    data.insert(interfaceExplicidDeviceC, m_interfaceExplicidDevice);
    data.insert(targetNameC, m_targetName);
    data.insert(targetDisableStackC, m_targetDisableStack);
    data.insert(gdbShutDownAfterDisconnectC, m_gdbShutDownAfterDisconnect);
    data.insert(gdbNotUseCacheC, m_gdbNotUseCache);

    return data;
}

bool EBlinkGdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!GdbServerProvider::fromMap(data))
        return false;

    m_executableFile = FilePath::fromVariant(data.value(executableFileKeyC));
    m_verboseLevel = data.value(verboseLevelKeyC).toInt();
    m_interfaceResetOnConnect = data.value(interfaceResetOnConnectC).toBool();
    m_interfaceType = static_cast<InterfaceType>(
                data.value(interfaceTypeC).toInt());
    m_deviceScript = FilePath::fromVariant(data.value(deviceScriptC));
    m_interfaceResetOnConnect = data.value(interfaceResetOnConnectC).toBool();
    m_interfaceSpeed = data.value(interfaceSpeedC).toInt();
    m_interfaceExplicidDevice = data.value(interfaceExplicidDeviceC).toString();
    m_targetName = data.value(targetNameC).toString();
    m_targetDisableStack = data.value(targetDisableStackC).toBool();
    m_gdbShutDownAfterDisconnect = data.value(gdbShutDownAfterDisconnectC).toBool();
    m_gdbNotUseCache = data.value(gdbNotUseCacheC).toBool();

    return true;
}

bool EBlinkGdbServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const EBlinkGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_verboseLevel == p->m_verboseLevel
            && m_interfaceType == p->m_interfaceType
            && m_deviceScript == p->m_deviceScript
            && m_interfaceResetOnConnect == p->m_interfaceResetOnConnect
            && m_interfaceSpeed == p->m_interfaceSpeed
            && m_interfaceExplicidDevice == p->m_interfaceExplicidDevice
            && m_targetName == p->m_targetName
            && m_targetDisableStack == p->m_targetDisableStack
            && m_gdbShutDownAfterDisconnect == p->m_gdbShutDownAfterDisconnect
            && m_gdbNotUseCache == p->m_gdbNotUseCache;
}

// EBlinkGdbServerProviderFactory

EBlinkGdbServerProviderFactory::EBlinkGdbServerProviderFactory()
{
    setId(Constants::GDBSERVER_EBLINK_PROVIDER_ID);
    setDisplayName(GdbServerProvider::tr("EBlink"));
    setCreator([] { return new EBlinkGdbServerProvider; });
}

// EBlinkGdbServerProviderConfigWidget

EBlinkGdbServerProviderConfigWidget::EBlinkGdbServerProviderConfigWidget(
        EBlinkGdbServerProvider *p)
    : GdbServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_gdbHostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_gdbHostWidget);

    m_executableFileChooser = new PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_mainLayout->addRow(tr("Executable file:"), m_executableFileChooser);

    m_scriptFileChooser = new Utils::PathChooser;
    m_scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_scriptFileChooser->setPromptDialogFilter("*.script");
    m_mainLayout->addRow(tr("Script file:"), m_scriptFileChooser);

    m_verboseLevelSpinBox = new QSpinBox;
    m_verboseLevelSpinBox->setRange(0, 7);
    m_verboseLevelSpinBox->setMaximumWidth(80);
    m_verboseLevelSpinBox->setToolTip(tr("Specify the verbosity level (0 to 7)."));
    m_mainLayout->addRow(tr("Verbosity level:"), m_verboseLevelSpinBox);

    m_resetOnConnectCheckBox = new QCheckBox;
    m_resetOnConnectCheckBox->setToolTip(tr("Connect under reset (hotplug)."));
    m_mainLayout->addRow(tr("Connect under reset:"), m_resetOnConnectCheckBox);

    m_interfaceTypeComboBox = new QComboBox;
    m_interfaceTypeComboBox->setToolTip(tr("Interface type."));
    m_mainLayout->addRow(tr("Type:"), m_interfaceTypeComboBox);

    m_interfaceSpeedSpinBox = new QSpinBox;
    m_interfaceSpeedSpinBox->setRange(120, 8000);
    m_interfaceSpeedSpinBox->setMaximumWidth(120);
    m_interfaceSpeedSpinBox->setToolTip(tr("Specify the speed of the interface (120 to 8000) in kilohertz (kHz)."));
    m_mainLayout->addRow(tr("Speed:"), m_interfaceSpeedSpinBox);

    m_notUseCacheCheckBox = new QCheckBox;
    m_notUseCacheCheckBox->setToolTip(tr("Do not use EBlink flash cache."));
    m_mainLayout->addRow(tr("Disable cache:"), m_notUseCacheCheckBox);

    m_shutDownAfterDisconnectCheckBox = new QCheckBox;
    m_shutDownAfterDisconnectCheckBox->setEnabled(false);
    m_shutDownAfterDisconnectCheckBox->setToolTip(tr("Shut down EBlink server after disconnect."));
    m_mainLayout->addRow(tr("Auto shutdown:"), m_shutDownAfterDisconnectCheckBox);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);

    populateInterfaceTypes();
    addErrorLabel();
    setFromProvider();

    const auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit);
    chooser->addSupportedWidget(m_resetCommandsTextEdit);

    connect(m_gdbHostWidget, &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_executableFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_scriptFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_verboseLevelSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_interfaceSpeedSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_notUseCacheCheckBox, &QAbstractButton::clicked,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_shutDownAfterDisconnectCheckBox, &QAbstractButton::clicked,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetOnConnectCheckBox, &QAbstractButton::clicked,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_interfaceTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_initCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
}

EBlinkGdbServerProvider::InterfaceType
EBlinkGdbServerProviderConfigWidget::interfaceTypeToWidget(int idx) const
{
    m_interfaceTypeComboBox->setCurrentIndex(idx);
    return interfaceTypeFromWidget();
}

EBlinkGdbServerProvider::InterfaceType
EBlinkGdbServerProviderConfigWidget::interfaceTypeFromWidget() const
{
    return static_cast<EBlinkGdbServerProvider::InterfaceType>(
                             m_interfaceTypeComboBox->currentIndex());
}

void EBlinkGdbServerProviderConfigWidget::populateInterfaceTypes()
{
    m_interfaceTypeComboBox->insertItem(EBlinkGdbServerProvider::SWD, tr("SWD"),
                EBlinkGdbServerProvider::SWD);
    m_interfaceTypeComboBox->insertItem(EBlinkGdbServerProvider::JTAG, tr("JTAG"),
                EBlinkGdbServerProvider::JTAG);
}

void EBlinkGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<EBlinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    m_gdbHostWidget->setChannel(p->channel());
    m_executableFileChooser->setFilePath(p->m_executableFile);
    m_verboseLevelSpinBox->setValue(p->m_verboseLevel);
    m_scriptFileChooser->setFilePath(p->m_deviceScript);
    m_interfaceTypeComboBox->setCurrentIndex(p->m_interfaceType);
    m_resetOnConnectCheckBox->setChecked(p->m_interfaceResetOnConnect);
    m_interfaceSpeedSpinBox->setValue(p->m_interfaceSpeed);
    m_shutDownAfterDisconnectCheckBox->setChecked(p->m_gdbShutDownAfterDisconnect);
    m_notUseCacheCheckBox->setChecked(p->m_gdbNotUseCache);

    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}


void EBlinkGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<EBlinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_gdbHostWidget->channel());
    p->m_executableFile = m_executableFileChooser->filePath();
    p->m_verboseLevel = m_verboseLevelSpinBox->value();
    p->m_deviceScript = m_scriptFileChooser->filePath();
    p->m_interfaceType = interfaceTypeFromWidget();
    p->m_interfaceResetOnConnect = m_resetOnConnectCheckBox->isChecked();
    p->m_interfaceSpeed = m_interfaceSpeedSpinBox->value();
    p->m_gdbShutDownAfterDisconnect = m_shutDownAfterDisconnectCheckBox->isChecked();
    p->m_gdbNotUseCache = m_notUseCacheCheckBox->isChecked();

    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
    GdbServerProviderConfigWidget::apply();
}

void EBlinkGdbServerProviderConfigWidget::discard()
{
    setFromProvider();
    GdbServerProviderConfigWidget::discard();
}

} // namespace Internal
} // namespace BareMetal
