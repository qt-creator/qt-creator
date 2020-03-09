/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <baremetal/idebugserverprovider.h>

#include <projectexplorer/runcontrol.h>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

// GdbServerProvider

class GdbServerProvider : public IDebugServerProvider
{
    Q_DECLARE_TR_FUNCTIONS(BareMetal::Internal::GdbServerProvider)

public:
    enum StartupMode {
        StartupOnNetwork,
        StartupOnPipe
    };

    StartupMode startupMode() const;
    Utils::FilePath peripheralDescriptionFile() const;
    QString initCommands() const;
    QString resetCommands() const;
    bool useExtendedRemote() const;

    bool operator==(const IDebugServerProvider &other) const override;

    QVariantMap toMap() const override;

    virtual Utils::CommandLine command() const;

    bool aboutToRun(Debugger::DebuggerRunTool *runTool,
                    QString &errorMessage) const final;
    ProjectExplorer::RunWorker *targetRunner(
            ProjectExplorer::RunControl *runControl) const final;

    bool isValid() const override;
    virtual QSet<StartupMode> supportedStartupModes() const = 0;

protected:
    explicit GdbServerProvider(const QString &id);
    explicit GdbServerProvider(const GdbServerProvider &other);

    void setStartupMode(StartupMode);
    void setPeripheralDescriptionFile(const Utils::FilePath &file);
    void setInitCommands(const QString &);
    void setResetCommands(const QString &);
    void setUseExtendedRemote(bool);

    bool fromMap(const QVariantMap &data) override;

    StartupMode m_startupMode = StartupOnNetwork;
    Utils::FilePath m_peripheralDescriptionFile;
    QString m_initCommands;
    QString m_resetCommands;
    bool m_useExtendedRemote = false;

    friend class GdbServerProviderConfigWidget;
};

// GdbServerProviderConfigWidget

class GdbServerProviderConfigWidget : public IDebugServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit GdbServerProviderConfigWidget(GdbServerProvider *provider);
    void apply() override;
    void discard() override;

protected:
    GdbServerProvider::StartupMode startupModeFromIndex(int idx) const;
    GdbServerProvider::StartupMode startupMode() const;
    void setStartupMode(GdbServerProvider::StartupMode mode);
    void populateStartupModes();
    Utils::FilePath peripheralDescriptionFile() const;
    void setPeripheralDescriptionFile(const Utils::FilePath &file);
    void setFromProvider();

    static QString defaultInitCommandsTooltip();
    static QString defaultResetCommandsTooltip();

    QComboBox *m_startupModeComboBox = nullptr;
    Utils::PathChooser *m_peripheralDescriptionFileChooser = nullptr;
};

// GdbServerProviderRunner

class GdbServerProviderRunner final : public ProjectExplorer::SimpleTargetRunner
{
public:
    explicit GdbServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                     const ProjectExplorer::Runnable &runnable);
};

} // namespace Internal
} // namespace BareMetal
