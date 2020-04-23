/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvproject.h"
#include "uvprojectwriter.h"
#include "uvtargetdeviceviewer.h"
#include "uvtargetdriverviewer.h"

#include "uvscserverprovider.h"

#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/baremetaldevice.h>
#include <baremetal/debugserverprovidermanager.h>

#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFormLayout>
#include <QRegularExpressionValidator>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

using namespace Uv;

// Whole software package selection keys.
constexpr char toolsIniKeyC[] = "BareMetal.UvscServerProvider.ToolsIni";
constexpr char deviceSelectionKeyC[] = "BareMetal.UvscServerProvider.DeviceSelection";
constexpr char driverSelectionKeyC[] = "BareMetal.UvscServerProvider.DriverSelection";

constexpr int defaultPortNumber = 5101;

// UvscServerProvider

QString UvscServerProvider::buildDllRegistryKey(const DriverSelection &driver)
{
    const QFileInfo fi(driver.dll);
    return fi.baseName();
}

QString UvscServerProvider::adjustFlashAlgorithmProperty(const QString &property)
{
    return property.startsWith("0x") ? property.mid(2) : property;
}

UvscServerProvider::UvscServerProvider(const QString &id)
    : IDebugServerProvider(id)
{
    setEngineType(UvscEngineType);
    setChannel("localhost", defaultPortNumber);
    setToolsetNumber(ArmAdsToolsetNumber);
}

UvscServerProvider::UvscServerProvider(const UvscServerProvider &other)
    : IDebugServerProvider(other.id())
{
    setEngineType(UvscEngineType);
}

void UvscServerProvider::setToolsIniFile(const Utils::FilePath &toolsIniFile)
{
    m_toolsIniFile = toolsIniFile;
}

Utils::FilePath UvscServerProvider::toolsIniFile() const
{
    return m_toolsIniFile;
}

void UvscServerProvider::setDeviceSelection(const DeviceSelection &deviceSelection)
{
    m_deviceSelection = deviceSelection;
}

DeviceSelection UvscServerProvider::deviceSelection() const
{
    return m_deviceSelection;
}

void UvscServerProvider::setDriverSelection(const DriverSelection &driverSelection)
{
    m_driverSelection = driverSelection;
}

DriverSelection UvscServerProvider::driverSelection() const
{
    return m_driverSelection;
}

void UvscServerProvider::setToolsetNumber(ToolsetNumber toolsetNumber)
{
    m_toolsetNumber = toolsetNumber;
}

UvscServerProvider::ToolsetNumber UvscServerProvider::toolsetNumber() const
{
    return m_toolsetNumber;
}

void UvscServerProvider::setSupportedDrivers(const QStringList &supportedDrivers)
{
    m_supportedDrivers = supportedDrivers;
}

QStringList UvscServerProvider::supportedDrivers() const
{
    return m_supportedDrivers;
}

bool UvscServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!IDebugServerProvider::operator==(other))
        return false;
    const auto p = static_cast<const UvscServerProvider *>(&other);
    return m_toolsIniFile == p->m_toolsIniFile
            && m_deviceSelection == p->m_deviceSelection
            && m_driverSelection == p->m_driverSelection
            && m_toolsetNumber == p->m_toolsetNumber;
}

FilePath UvscServerProvider::buildProjectFilePath(DebuggerRunTool *runTool) const
{
    const RunControl *control = runTool->runControl();
    const QString projectName = control->project()->displayName() + ".uvprojx";
    const FilePath path = control->buildDirectory().pathAppended(projectName);
    return path;
}

FilePath UvscServerProvider::buildOptionsFilePath(DebuggerRunTool *runTool) const
{
    const RunControl *control = runTool->runControl();
    const QString projectName = control->project()->displayName() + ".uvoptx";
    const FilePath path = control->buildDirectory().pathAppended(projectName);
    return path;
}

QVariantMap UvscServerProvider::toMap() const
{
    QVariantMap data = IDebugServerProvider::toMap();
    data.insert(toolsIniKeyC, m_toolsIniFile.toVariant());
    data.insert(deviceSelectionKeyC, m_deviceSelection.toMap());
    data.insert(driverSelectionKeyC, m_driverSelection.toMap());
    return data;
}

bool UvscServerProvider::isValid() const
{
    return m_channel.isValid();
}

QString UvscServerProvider::channelString() const
{
    return m_channel.toString();
}

bool UvscServerProvider::aboutToRun(DebuggerRunTool *runTool, QString &errorMessage) const
{
    QTC_ASSERT(runTool, return false);
    const RunControl *runControl = runTool->runControl();
    const auto exeAspect = runControl->aspect<ExecutableAspect>();
    QTC_ASSERT(exeAspect, return false);

    const FilePath bin = exeAspect->executable();
    if (bin.isEmpty()) {
        errorMessage = BareMetalDebugSupport::tr("Cannot debug: Local executable is not set.");
        return false;
    } else if (!bin.exists()) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Cannot debug: Could not find executable for \"%1\".").arg(bin.toString());
        return false;
    }

    const FilePath projFilePath = projectFilePath(runTool, errorMessage);
    if (!projFilePath.exists())
        return false;

    const FilePath optFilePath = optionsFilePath(runTool, errorMessage);
    if (!optFilePath.exists())
        return false;

    const FilePath peripheralDescriptionFile = FilePath::fromString(m_deviceSelection.svd);

    Runnable inferior;
    inferior.executable = bin;
    inferior.extraData.insert(Debugger::Constants::kPeripheralDescriptionFile,
                              peripheralDescriptionFile.toVariant());
    inferior.extraData.insert(Debugger::Constants::kUVisionProjectFilePath, projFilePath.toString());
    inferior.extraData.insert(Debugger::Constants::kUVisionOptionsFilePath, optFilePath.toString());
    inferior.extraData.insert(Debugger::Constants::kUVisionSimulator, isSimulator());
    runTool->setInferior(inferior);
    runTool->setSymbolFile(bin);
    runTool->setStartMode(AttachToRemoteServer);
    runTool->setRemoteChannel(channelString());
    runTool->setUseContinueInsteadOfRun(true);
    return true;
}

ProjectExplorer::RunWorker *UvscServerProvider::targetRunner(RunControl *runControl) const
{
    // Get uVision executable path.
    const Runnable uv = DebuggerKitAspect::runnable(runControl->kit());
    CommandLine server(uv.executable);
    server.addArg("-j0");
    server.addArg(QStringLiteral("-s%1").arg(m_channel.port()));

    Runnable r;
    r.setCommandLine(server);
    return new UvscServerProviderRunner(runControl, r);
}

bool UvscServerProvider::fromMap(const QVariantMap &data)
{
    if (!IDebugServerProvider::fromMap(data))
        return false;
    m_toolsIniFile = FilePath::fromVariant(data.value(toolsIniKeyC));
    m_deviceSelection.fromMap(data.value(deviceSelectionKeyC).toMap());
    m_driverSelection.fromMap(data.value(driverSelectionKeyC).toMap());
    return true;
}

Utils::FilePath UvscServerProvider::projectFilePath(DebuggerRunTool *runTool,
                                                    QString &errorMessage) const
{
    const FilePath projectPath = buildProjectFilePath(runTool);
    std::ofstream ofs(projectPath.toString().toStdString(), std::ofstream::out);
    Uv::ProjectWriter writer(&ofs);
    const Uv::Project project(this, runTool);
    if (!writer.write(&project)) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Unable to create a uVision project template.");
        return {};
    }
    return projectPath;
}

// HexValueValidator

class HexValueValidator final : public QRegularExpressionValidator
{
public:
    explicit HexValueValidator(QObject *parent = nullptr)
        : QRegularExpressionValidator(parent)
    {
        const QRegularExpression re("^0x[0-9a-fA-F]{1,8}");
        setRegularExpression(re);
    }
};

// UvscServerProviderConfigWidget

UvscServerProviderConfigWidget::UvscServerProviderConfigWidget(UvscServerProvider *provider)
    : IDebugServerProviderConfigWidget(provider)
{
    m_hostWidget = new HostWidget;
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);
    m_toolsIniChooser = new PathChooser;
    m_toolsIniChooser->setExpectedKind(PathChooser::File);
    m_toolsIniChooser->setPromptDialogFilter("tools.ini");
    m_toolsIniChooser->setPromptDialogTitle(tr("Choose Keil Toolset Configuration File"));
    m_mainLayout->addRow(tr("Tools file path:"), m_toolsIniChooser);
    m_deviceSelector = new DeviceSelector;
    m_mainLayout->addRow(tr("Target device:"), m_deviceSelector);
    m_driverSelector = new DriverSelector(provider->supportedDrivers());
    m_mainLayout->addRow(tr("Target driver:"), m_driverSelector);

    setFromProvider();

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_toolsIniChooser, &PathChooser::pathChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_deviceSelector, &DeviceSelector::selectionChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_driverSelector, &DriverSelector::selectionChanged,
            this, &UvscServerProviderConfigWidget::dirty);

    auto updateSelectors = [this]() {
        const FilePath toolsIniFile = m_toolsIniChooser->filePath();
        m_deviceSelector->setToolsIniFile(toolsIniFile);
        m_driverSelector->setToolsIniFile(toolsIniFile);
    };

    connect(m_toolsIniChooser, &PathChooser::pathChanged, updateSelectors);
    updateSelectors();
}

void UvscServerProviderConfigWidget::apply()
{
    const auto p = static_cast<UvscServerProvider *>(m_provider);
    p->setToolsIniFile(toolsIniFile());
    p->setDeviceSelection(deviceSelection());
    p->setDriverSelection(driverSelection());
    IDebugServerProviderConfigWidget::apply();
}

void UvscServerProviderConfigWidget::discard()
{
    setFromProvider();
    IDebugServerProviderConfigWidget::discard();
}

void UvscServerProviderConfigWidget::setToolsIniFile(const Utils::FilePath &toolsIniFile)
{
    m_toolsIniChooser->setFilePath(toolsIniFile);
}

Utils::FilePath UvscServerProviderConfigWidget::toolsIniFile() const
{
    return m_toolsIniChooser->filePath();
}

void UvscServerProviderConfigWidget::setDeviceSelection(const DeviceSelection &deviceSelection)
{
    m_deviceSelector->setSelection(deviceSelection);
}

DeviceSelection UvscServerProviderConfigWidget::deviceSelection() const
{
    return m_deviceSelector->selection();
}

void UvscServerProviderConfigWidget::setDriverSelection(const DriverSelection &driverSelection)
{
    m_driverSelector->setSelection(driverSelection);
}

DriverSelection UvscServerProviderConfigWidget::driverSelection() const
{
    return m_driverSelector->selection();
}

void UvscServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<UvscServerProvider *>(m_provider);
    m_hostWidget->setChannel(p->channel());
    m_toolsIniChooser->setFilePath(p->toolsIniFile());
    m_deviceSelector->setSelection(p->deviceSelection());
    m_driverSelector->setSelection(p->driverSelection());
}

// UvscServerProviderRunner

UvscServerProviderRunner::UvscServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                                   const Runnable &runnable)
    : RunWorker(runControl)
{
    setId("BareMetalUvscServer");

    const QString program = runnable.executable.toString();
    const QStringList args = runnable.commandLineArguments.split(' ');
    m_process.setProgram(program);
    m_process.setArguments(args);

    connect(&m_process, &QtcProcess::started, this, [this]() {
        ProcessHandle pid(m_process.processId());
        this->runControl()->setApplicationProcessHandle(pid);
        reportStarted();
    });
    connect(&m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QtcProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus status) {
                const QString msg = (status == QProcess::CrashExit)
                                        ? RunControl::tr("%1 crashed.")
                                        : RunControl::tr("%2 exited with code %1").arg(exitCode);
                appendMessage(msg.arg(m_process.program()), Utils::NormalMessageFormat);
                reportStopped();
            });
    connect(&m_process, &QtcProcess::errorOccurred, this, [this] (QProcess::ProcessError error) {
        if (error == QProcess::Timedout)
            return; // No actual change on the process side.
        const QString msg = userMessageForProcessError(
                    error, FilePath::fromString(m_process.program()));
        appendMessage(msg, Utils::NormalMessageFormat);
        reportStopped();
    });
}

void UvscServerProviderRunner::start()
{
    const QString msg = RunControl::tr("Starting %1 %2...")
            .arg(m_process.program()).arg(m_process.arguments().join(' '));
    appendMessage(msg, Utils::NormalMessageFormat);

    m_process.start();
}

void UvscServerProviderRunner::stop()
{
    m_process.terminate();
}

} // namespace Internal
} // namespace BareMetal
