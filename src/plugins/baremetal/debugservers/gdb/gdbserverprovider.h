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
class QSpinBox;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

// GdbServerProvider

class GdbServerProvider : public IDebugServerProvider
{
public:
    enum StartupMode {
        NoStartup = 0,
        StartupOnNetwork,
        StartupOnPipe,
        StartupModesCount
    };

    StartupMode startupMode() const;
    QString initCommands() const;
    QString resetCommands() const;
    bool useExtendedRemote() const;

    bool operator==(const IDebugServerProvider &other) const override;

    virtual QString channelString() const;

    QVariantMap toMap() const override;

    virtual Utils::CommandLine command() const;

    bool aboutToRun(Debugger::DebuggerRunTool *runTool,
                    QString &errorMessage) const final;
    ProjectExplorer::RunWorker *targetRunner(
            ProjectExplorer::RunControl *runControl) const final;

    bool isValid() const override;
    virtual bool canStartupMode(StartupMode) const;

    QUrl channel() const;
    void setChannel(const QUrl &channelString);
    void setDefaultChannel(const QString &host, int port);

protected:
    explicit GdbServerProvider(const QString &id);
    explicit GdbServerProvider(const GdbServerProvider &other);

    void setStartupMode(StartupMode);
    void setInitCommands(const QString &);
    void setResetCommands(const QString &);
    void setUseExtendedRemote(bool);
    void setSettingsKeyBase(const QString &settingsBase);

    bool fromMap(const QVariantMap &data) override;

    QString m_settingsBase;
    QUrl m_channel;
    StartupMode m_startupMode = NoStartup;
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
    void setFromProvider();

    static QString defaultInitCommandsTooltip();
    static QString defaultResetCommandsTooltip();

    QComboBox *m_startupModeComboBox = nullptr;
};

// GdbServerProviderRunner

class GdbServerProviderRunner final : public ProjectExplorer::SimpleTargetRunner
{
public:
    explicit GdbServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                     const ProjectExplorer::Runnable &runnable);
};

// HostWidget

class HostWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit HostWidget(QWidget *parent = nullptr);

    void setChannel(const QUrl &host);
    QUrl channel() const;

signals:
    void dataChanged();

protected:
    QLineEdit *m_hostLineEdit = nullptr;
    QSpinBox *m_portSpinBox = nullptr;
};

} // namespace Internal
} // namespace BareMetal
