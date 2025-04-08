// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"
#include "uvtargetdeviceviewer.h"
#include "uvtargetdriverviewer.h"

#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/baremetaldevice.h>
#include <baremetal/baremetaltr.h>
#include <baremetal/debugserverprovidermanager.h>

#include <debugger/debuggerengine.h>
#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QFormLayout>
#include <QRegularExpressionValidator>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

using namespace Uv;

// Whole software package selection keys.
constexpr char toolsIniKeyC[] = "ToolsIni";
constexpr char deviceSelectionKeyC[] = "DeviceSelection";
constexpr char driverSelectionKeyC[] = "DriverSelection";

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

void UvscServerProvider::setToolsIniFile(const FilePath &toolsIniFile)
{
    m_toolsIniFile = toolsIniFile;
}

FilePath UvscServerProvider::toolsIniFile() const
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

FilePath UvscServerProvider::buildProjectFilePath(RunControl *runControl) const
{
    const QString projectName = runControl->project()->displayName() + ".uvprojx";
    const FilePath path = runControl->buildDirectory().pathAppended(projectName);
    return path;
}

FilePath UvscServerProvider::buildOptionsFilePath(RunControl *runControl) const
{
    const QString projectName = runControl->project()->displayName() + ".uvoptx";
    const FilePath path = runControl->buildDirectory().pathAppended(projectName);
    return path;
}

void UvscServerProvider::toMap(Store &data) const
{
    IDebugServerProvider::toMap(data);
    data.insert(toolsIniKeyC, m_toolsIniFile.toSettings());
    data.insert(deviceSelectionKeyC, variantFromStore(m_deviceSelection.toMap()));
    data.insert(driverSelectionKeyC, variantFromStore(m_driverSelection.toMap()));
}

bool UvscServerProvider::isValid() const
{
    return m_channel.isValid();
}

QString UvscServerProvider::channelString() const
{
    return m_channel.toString();
}

Result<> UvscServerProvider::setupDebuggerRunParameters(DebuggerRunParameters &rp,
                                                      RunControl *runControl) const
{
    const FilePath bin = rp.inferior().command.executable();
    if (bin.isEmpty()) {
        return ResultError(Tr::tr("Cannot debug: Local executable is not set."));
    } else if (!bin.exists()) {
        return ResultError(Tr::tr("Cannot debug: Could not find executable for \"%1\".")
                                 .arg(bin.toUserOutput()));
    }

    QString errorMessage;
    const FilePath projFilePath = projectFilePath(runControl, errorMessage);
    if (!projFilePath.exists())
        return ResultError(errorMessage);

    const FilePath optFilePath = optionsFilePath(runControl, errorMessage);
    if (!optFilePath.exists())
        return ResultError(errorMessage);

    const FilePath peripheralDescriptionFile = FilePath::fromString(m_deviceSelection.svd);

    ProcessRunData inferior;
    inferior.command.setExecutable(bin);
    rp.setPeripheralDescriptionFile(peripheralDescriptionFile);
    rp.setUVisionProjectFilePath(projFilePath);
    rp.setUVisionOptionsFilePath(optFilePath);
    rp.setUVisionSimulator(isSimulator());
    rp.setInferior(inferior);
    rp.setSymbolFile(bin);
    rp.setStartMode(AttachToRemoteServer);
    rp.setRemoteChannel(channelString());
    rp.setUseContinueInsteadOfRun(true);
    return ResultOk;
}

ProjectExplorer::RunWorker *UvscServerProvider::targetRunner(RunControl *runControl) const
{
    return createProcessWorker(runControl, [this, runControl](Process &process) {
        process.setCommand({DebuggerKitAspect::runnable(runControl->kit()).command.executable(),
                            {"-j0", QStringLiteral("-s%1").arg(m_channel.port())}});
    });
}

void UvscServerProvider::fromMap(const Store &data)
{
    IDebugServerProvider::fromMap(data);
    m_toolsIniFile = FilePath::fromSettings(data.value(toolsIniKeyC));
    m_deviceSelection.fromMap(storeFromVariant(data.value(deviceSelectionKeyC)));
    m_driverSelection.fromMap(storeFromVariant(data.value(driverSelectionKeyC)));
}

FilePath UvscServerProvider::projectFilePath(RunControl *runControl, QString &errorMessage) const
{
    const FilePath projectPath = buildProjectFilePath(runControl);
    std::ofstream ofs(projectPath.path().toStdString(), std::ofstream::out);
    Uv::ProjectWriter writer(&ofs);
    const Uv::Project project(this, runControl->project());
    if (!writer.write(&project)) {
        errorMessage = Tr::tr("Unable to create a uVision project template.");
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
        static const QRegularExpression re("^0x[0-9a-fA-F]{1,8}");
        setRegularExpression(re);
    }
};

// UvscServerProviderConfigWidget

UvscServerProviderConfigWidget::UvscServerProviderConfigWidget(UvscServerProvider *provider)
    : IDebugServerProviderConfigWidget(provider)
{
    m_hostWidget = new HostWidget;
    m_mainLayout->addRow(Tr::tr("Host:"), m_hostWidget);
    m_toolsIniChooser = new PathChooser;
    m_toolsIniChooser->setExpectedKind(PathChooser::File);
    m_toolsIniChooser->setPromptDialogFilter("tools.ini");
    m_toolsIniChooser->setPromptDialogTitle(Tr::tr("Choose Keil Toolset Configuration File"));
    m_mainLayout->addRow(Tr::tr("Tools file path:"), m_toolsIniChooser);
    m_deviceSelector = new DeviceSelector;
    m_mainLayout->addRow(Tr::tr("Target device:"), m_deviceSelector);
    m_driverSelector = new DriverSelector(provider->supportedDrivers());
    m_mainLayout->addRow(Tr::tr("Target driver:"), m_driverSelector);

    setFromProvider();

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_toolsIniChooser, &PathChooser::textChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_deviceSelector, &DeviceSelector::selectionChanged,
            this, &UvscServerProviderConfigWidget::dirty);
    connect(m_driverSelector, &DriverSelector::selectionChanged,
            this, &UvscServerProviderConfigWidget::dirty);

    auto updateSelectors = [this] {
        const FilePath toolsIniFile = m_toolsIniChooser->filePath();
        m_deviceSelector->setToolsIniFile(toolsIniFile);
        m_driverSelector->setToolsIniFile(toolsIniFile);
    };

    connect(m_toolsIniChooser, &PathChooser::textChanged, this, updateSelectors);
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

void UvscServerProviderConfigWidget::setToolsIniFile(const FilePath &toolsIniFile)
{
    m_toolsIniChooser->setFilePath(toolsIniFile);
}

FilePath UvscServerProviderConfigWidget::toolsIniFile() const
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

} // BareMetal::Internal
