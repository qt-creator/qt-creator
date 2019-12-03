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
const char gdbPortC[] = "BareMetal.EBlinkGdbServerProvider.GdbPort";
const char gdbAddressC[] = "BareMetal.EBlinkGdbServerProvider.GdbAddress";

// EBlinkGdbServerProvider

EBlinkGdbServerProvider::EBlinkGdbServerProvider()
    : GdbServerProvider(Constants::EBLINK_PROVIDER_ID)
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setDefaultChannel("127.0.0.1", 2331);
    setSettingsKeyBase("BareMetal.EBlinkGdbServerProvider");
    setTypeDisplayName(EBlinkGdbServerProviderFactory::tr("EBlink"));
}

EBlinkGdbServerProvider::EBlinkGdbServerProvider(
        const EBlinkGdbServerProvider &other)
    : GdbServerProvider(other)
    , m_executableFile(other.m_executableFile)
    , m_verboseLevel(0)
    , m_interfaceType(other.m_interfaceType)
    , m_deviceScript(other.m_deviceScript)
    , m_interfaceResetOnConnect(other.m_interfaceResetOnConnect)
    , m_interfaceSpeed(other.m_interfaceSpeed)
    , m_interfaceExplicidDevice(other.m_interfaceExplicidDevice)
    , m_targetName(other.m_targetName)
    , m_targetDisableStack(other.m_targetDisableStack)
    , m_gdbShutDownAfterDisconnect(other.m_gdbShutDownAfterDisconnect)
    , m_gdbNotUseCache(other.m_gdbNotUseCache){
}

QString EBlinkGdbServerProvider::defaultInitCommands()
{
    return {"monitor reset halt\n"
        "load\n"
        "monitor reset halt\n"};
}

QString EBlinkGdbServerProvider::defaultResetCommands()
{
    return {"monitor reset halt\n"};
}

QString EBlinkGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case NoStartup:
        // fallback
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    case StartupOnPipe:
        // Unsupported mode
        return {};
    default: // wrong
        return {};
    }
}

CommandLine EBlinkGdbServerProvider::command() const
{
    CommandLine cmd{m_executableFile, {}};
    QStringList interFaceTypeStrings = {"swd", "jtag"};

    //Mandotary -I
    cmd.addArg("-I");
    QString interfaceArgs("stlink,%1,speed=%2");
    interfaceArgs = interfaceArgs.arg(interFaceTypeStrings.at(m_interfaceType))
                                .arg(QString::number(m_interfaceSpeed));
    if (!m_interfaceResetOnConnect)
        interfaceArgs.append(",dr");
    if(!m_interfaceExplicidDevice.trimmed().isEmpty())
        interfaceArgs.append(",device="+m_interfaceExplicidDevice.trimmed());
    cmd.addArg(interfaceArgs);

    //Mandotary -D
    cmd.addArg("-D");
    cmd.addArg(m_deviceScript);

    //Mandotary -G
    cmd.addArg("-G");
    QString gdbArgs("port=%1,address=%2");
    gdbArgs = gdbArgs.arg(QString::number(channel().port()))
                    .arg(channel().host());
    if(m_gdbNotUseCache)
        gdbArgs.append(",nc");
    if(m_gdbShutDownAfterDisconnect)
        gdbArgs.append(",S");
    cmd.addArg(gdbArgs);

    cmd.addArg("-T");
    QString targetArgs(m_targetName.trimmed());
    if(m_targetDisableStack)
        targetArgs.append(",nu");
    cmd.addArg(targetArgs);

    cmd.addArg("-v");
    cmd.addArg(QString::number(m_verboseLevel));
#ifdef WIN32
    cmd.addArg("-g"); //no gui
#endif

    return cmd;
}

bool EBlinkGdbServerProvider::canStartupMode(StartupMode m) const
{
    return m == NoStartup || m == StartupOnNetwork;
}

bool EBlinkGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    const StartupMode m = startupMode();

    if (m == NoStartup || m == StartupOnNetwork) {
        if (channel().host().isEmpty())
            return false;
    }

    if (m == StartupOnNetwork) {
        if (m_executableFile.isEmpty())
            return false;
    }

    return true;
}

GdbServerProvider *EBlinkGdbServerProvider::clone() const
{
    return new EBlinkGdbServerProvider(*this);
}

QVariantMap EBlinkGdbServerProvider::toMap() const
{
    QVariantMap data = GdbServerProvider::toMap();
    data.insert(executableFileKeyC, m_executableFile.toVariant());
    data.insert(verboseLevelKeyC, m_verboseLevel);
    data.insert(interfaceTypeC, m_interfaceType);
    data.insert(deviceScriptC, m_deviceScript);
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
    m_deviceScript = data.value(deviceScriptC).toString();
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
            && m_interfaceResetOnConnect == p->m_interfaceResetOnConnect
            && m_interfaceType == p->m_interfaceType;
}

GdbServerProviderConfigWidget *EBlinkGdbServerProvider::configurationWidget()
{
    return new EBlinkGdbServerProviderConfigWidget(this);
}

// EBlinkGdbServerProviderFactory

EBlinkGdbServerProviderFactory::EBlinkGdbServerProviderFactory()
{
    setId(Constants::EBLINK_PROVIDER_ID);
    setDisplayName(tr("EBlink"));
}

GdbServerProvider *EBlinkGdbServerProviderFactory::create()
{
    return new EBlinkGdbServerProvider;
}

bool EBlinkGdbServerProviderFactory::canRestore(const QVariantMap &data) const
{
    const QString id = idFromMap(data);
    return id.startsWith(Constants::EBLINK_PROVIDER_ID + QLatin1Char(':'));
}

GdbServerProvider *EBlinkGdbServerProviderFactory::restore(const QVariantMap &data)
{
    const auto p = new EBlinkGdbServerProvider;
    const auto updated = data;
    if (p->fromMap(updated))
        return p;
    delete p;
    return nullptr;
}

// EBlinkGdbServerProviderConfigWidget

EBlinkGdbServerProviderConfigWidget::EBlinkGdbServerProviderConfigWidget(
        EBlinkGdbServerProvider *p)
    : GdbServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_gdbHostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_gdbHostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_mainLayout->addRow(tr("Executable file:"), m_executableFileChooser);

    m_deviceScriptLineEdit = new QLineEdit;
    m_deviceScriptLineEdit->setToolTip(tr("Device config script"));
    m_mainLayout->addRow(tr("Script:"), m_deviceScriptLineEdit);

    m_verboseLevelSpinBox = new QSpinBox;
    m_verboseLevelSpinBox->setRange(0,7);
    m_verboseLevelSpinBox->setMaximumWidth(80);
    m_verboseLevelSpinBox->setToolTip(tr("Specify the verbosity level (0..7)."));
    m_mainLayout->addRow(tr("Verbosity level:"), m_verboseLevelSpinBox);

    m_resetOnConnectCheckBox = new QCheckBox;
    m_resetOnConnectCheckBox->setToolTip(tr("Connect under reset (hotplug)."));
    m_mainLayout->addRow(tr("Connect under reset:"), m_resetOnConnectCheckBox);

    m_interfaceTypeComboBox = new QComboBox;
    m_interfaceTypeComboBox->setToolTip(tr("Interface type. SWD or JTAG"));
    m_mainLayout->addRow(tr("Type:"), m_interfaceTypeComboBox);

    m_interfaceSpeedSpinBox = new QSpinBox;
    m_interfaceSpeedSpinBox->setRange(120,8000);
    m_interfaceSpeedSpinBox->setMaximumWidth(120);
    m_interfaceSpeedSpinBox->setToolTip(tr("Specify speed of Interface (120-8000)kHz"));
    m_mainLayout->addRow(tr("Speed:"), m_interfaceSpeedSpinBox);

    m_notUseCacheCheckBox = new QCheckBox;
    m_notUseCacheCheckBox->setToolTip(tr("Don't use EBlink flash cache"));
    m_mainLayout->addRow(tr("Disable cache:"), m_notUseCacheCheckBox);

    m_shutDownAfterDisconnectCheckBox = new QCheckBox;
    m_shutDownAfterDisconnectCheckBox->setEnabled(false);
    m_shutDownAfterDisconnectCheckBox->setToolTip(tr("Shutdown EBlink server after disconnect"));
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

    connect(m_deviceScriptLineEdit,
            &QLineEdit::textChanged,
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

    connect(m_startupModeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EBlinkGdbServerProviderConfigWidget::startupModeChanged);
}

EBlinkGdbServerProvider::InterfaceType
EBlinkGdbServerProviderConfigWidget::interfaceTypeToWidget(int idx) const
{
    return static_cast<EBlinkGdbServerProvider::InterfaceType>(
                m_interfaceTypeComboBox->itemData(idx).toInt());
}

EBlinkGdbServerProvider::InterfaceType
EBlinkGdbServerProviderConfigWidget::interfaceTypeFromWidget() const
{
    const int idx = m_interfaceTypeComboBox->currentIndex();
    return interfaceTypeToWidget(idx);
}

void EBlinkGdbServerProviderConfigWidget::setInterfaceWidgetType(
        EBlinkGdbServerProvider::InterfaceType tl)
{
    for (int idx = 0; idx < m_interfaceTypeComboBox->count(); ++idx) {
        if (tl == interfaceTypeToWidget(idx)) {
            m_interfaceTypeComboBox->setCurrentIndex(idx);
            break;
        }
    }
}

void EBlinkGdbServerProviderConfigWidget::startupModeChanged()
{
    const GdbServerProvider::StartupMode m = startupMode();
    const bool isStartup = m != GdbServerProvider::NoStartup;
    m_executableFileChooser->setVisible(isStartup);
    m_mainLayout->labelForField(m_executableFileChooser)->setVisible(isStartup);
    m_verboseLevelSpinBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_verboseLevelSpinBox)->setVisible(isStartup);
    m_interfaceTypeComboBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_interfaceTypeComboBox)->setVisible(isStartup);
    m_interfaceSpeedSpinBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_interfaceSpeedSpinBox)->setVisible(isStartup);
    m_notUseCacheCheckBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_notUseCacheCheckBox)->setVisible(isStartup);
    m_shutDownAfterDisconnectCheckBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_shutDownAfterDisconnectCheckBox)->setVisible(isStartup);
    m_resetOnConnectCheckBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_resetOnConnectCheckBox)->setVisible(isStartup);
    m_deviceScriptLineEdit->setVisible(isStartup);
    m_mainLayout->labelForField(m_deviceScriptLineEdit)->setVisible(isStartup);
}

void EBlinkGdbServerProviderConfigWidget::populateInterfaceTypes()
{
    m_interfaceTypeComboBox->insertItem(
                m_interfaceTypeComboBox->count(), tr("SWD"),
                EBlinkGdbServerProvider::SWD);
    m_interfaceTypeComboBox->insertItem(
                m_interfaceTypeComboBox->count(), tr("JTAG"),
                EBlinkGdbServerProvider::JTAG);
}

void EBlinkGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<EBlinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    //const QSignalBlocker blocker(this);
    startupModeChanged();
    m_gdbHostWidget->setChannel(p->channel());
    m_executableFileChooser->setFileName(p->m_executableFile);
    m_verboseLevelSpinBox->setValue(p->m_verboseLevel);
    m_deviceScriptLineEdit->setText(p->m_deviceScript);
    m_interfaceTypeComboBox->setCurrentIndex(p->m_interfaceType);
    m_resetOnConnectCheckBox->setChecked(p->m_interfaceResetOnConnect);
    m_interfaceSpeedSpinBox->setValue(p->m_interfaceSpeed);
    //interfaceExplicidDeviceC
    //targetNameC
    //targetDisableStackC
    m_shutDownAfterDisconnectCheckBox->setChecked(p->m_gdbShutDownAfterDisconnect);
    m_notUseCacheCheckBox->setChecked(p->m_gdbNotUseCache);
    //
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}


void EBlinkGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<EBlinkGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_gdbHostWidget->channel());
    p->m_executableFile = m_executableFileChooser->fileName();
    p->m_verboseLevel = m_verboseLevelSpinBox->value();
    p->m_deviceScript = m_deviceScriptLineEdit->text().trimmed();
    p->m_interfaceType = interfaceTypeFromWidget();
    p->m_interfaceResetOnConnect = m_resetOnConnectCheckBox->isChecked();
    p->m_interfaceSpeed = m_interfaceSpeedSpinBox->value();
    //interfaceExplicidDeviceC
    //targetNameC
    //targetDisableStackC
    p->m_gdbShutDownAfterDisconnect = m_shutDownAfterDisconnectCheckBox->isChecked();
    p->m_gdbNotUseCache = m_notUseCacheCheckBox->isChecked();
    //
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
} // namespace ProjectExplorer
