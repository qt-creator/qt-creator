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

#pragma once

#include "uvtargetdeviceselection.h"
#include "uvtargetdriverselection.h"

#include <baremetal/idebugserverprovider.h>

#include <projectexplorer/runcontrol.h> // for RunWorker

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

namespace Uv {
class DeviceSelection;
class DeviceSelector;
class DriverSelection;
class DriverSelector;
}

// UvscServerProvider

class UvscServerProvider : public IDebugServerProvider
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::UvscServerProvider)

public:
    enum ToolsetNumber {
        UnknownToolsetNumber = -1,
        ArmAdsToolsetNumber = 4 // ARM-ADS toolset
    };

    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;

    void setDeviceSelection(const Uv::DeviceSelection &deviceSelection);
    Uv::DeviceSelection deviceSelection() const;

    void setDriverSelection(const Uv::DriverSelection &driverSelection);
    Uv::DriverSelection driverSelection() const;

    ToolsetNumber toolsetNumber() const;
    QStringList supportedDrivers() const;

    bool operator==(const IDebugServerProvider &other) const override;

    QVariantMap toMap() const override;

    bool aboutToRun(Debugger::DebuggerRunTool *runTool, QString &errorMessage) const final;
    ProjectExplorer::RunWorker *targetRunner(ProjectExplorer::RunControl *runControl) const final;

    bool isValid() const override;
    QString channelString() const final;

    static QString buildDllRegistryKey(const Uv::DriverSelection &driver);
    static QString adjustFlashAlgorithmProperty(const QString &property);

protected:
    explicit UvscServerProvider(const QString &id);
    explicit UvscServerProvider(const UvscServerProvider &other);

    void setToolsetNumber(ToolsetNumber toolsetNumber);
    void setSupportedDrivers(const QStringList &supportedDrivers);

    Utils::FilePath buildProjectFilePath(Debugger::DebuggerRunTool *runTool) const;
    Utils::FilePath buildOptionsFilePath(Debugger::DebuggerRunTool *runTool) const;

    bool fromMap(const QVariantMap &data) override;

    // uVision specific stuff.
    virtual Utils::FilePath projectFilePath(Debugger::DebuggerRunTool *runTool,
                                            QString &errorMessage) const;
    virtual Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                            QString &errorMessage) const = 0;

    Utils::FilePath m_toolsIniFile;
    Uv::DeviceSelection m_deviceSelection;
    Uv::DriverSelection m_driverSelection;

    // Note: Don't store it to the map!
    ToolsetNumber m_toolsetNumber = UnknownToolsetNumber;
    QStringList m_supportedDrivers;

    friend class UvscServerProviderConfigWidget;
};

// UvscServerProviderConfigWidget

class UvscServerProviderConfigWidget : public IDebugServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit UvscServerProviderConfigWidget(UvscServerProvider *provider);
    void apply() override;
    void discard() override;

protected:
    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;
    void setDeviceSelection(const Uv::DeviceSelection &deviceSelection);
    Uv::DeviceSelection deviceSelection() const;
    void setDriverSelection(const Uv::DriverSelection &driverSelection);
    Uv::DriverSelection driverSelection() const;

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_toolsIniChooser = nullptr;
    Uv::DeviceSelector *m_deviceSelector = nullptr;
    Uv::DriverSelector *m_driverSelector = nullptr;
};

// UvscServerProviderRunner

class UvscServerProviderRunner final : public ProjectExplorer::RunWorker
{
public:
    explicit UvscServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                      const ProjectExplorer::Runnable &runnable);

private:
    void start() final;
    void stop() final;

    QProcess m_process;
};

} // namespace Internal
} // namespace BareMetal
