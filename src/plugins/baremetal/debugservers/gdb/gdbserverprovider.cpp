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

#include "gdbserverprovider.h"

#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/baremetaldevice.h>
#include <baremetal/debugserverprovidermanager.h>

#include <projectexplorer/runconfigurationaspects.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <ssh/sshconnection.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

const char startupModeKeyC[] = "BareMetal.GdbServerProvider.Mode";
const char initCommandsKeyC[] = "BareMetal.GdbServerProvider.InitCommands";
const char resetCommandsKeyC[] = "BareMetal.GdbServerProvider.ResetCommands";
const char useExtendedRemoteKeyC[] = "BareMetal.GdbServerProvider.UseExtendedRemote";

// GdbServerProvider

GdbServerProvider::GdbServerProvider(const QString &id)
    : IDebugServerProvider(id)
{
    setEngineType(Debugger::GdbEngineType);
}

GdbServerProvider::GdbServerProvider(const GdbServerProvider &other)
    : IDebugServerProvider(other.id())
    , m_startupMode(other.m_startupMode)
    , m_initCommands(other.m_initCommands)
    , m_resetCommands(other.m_resetCommands)
    , m_useExtendedRemote(other.useExtendedRemote())
{
    setEngineType(Debugger::GdbEngineType);
}

GdbServerProvider::StartupMode GdbServerProvider::startupMode() const
{
    return m_startupMode;
}

void GdbServerProvider::setStartupMode(StartupMode m)
{
    m_startupMode = m;
}

QString GdbServerProvider::initCommands() const
{
    return m_initCommands;
}

void GdbServerProvider::setInitCommands(const QString &cmds)
{
    m_initCommands = cmds;
}

bool GdbServerProvider::useExtendedRemote() const
{
    return m_useExtendedRemote;
}

void GdbServerProvider::setUseExtendedRemote(bool useExtendedRemote)
{
    m_useExtendedRemote = useExtendedRemote;
}

QString GdbServerProvider::resetCommands() const
{
    return m_resetCommands;
}

void GdbServerProvider::setResetCommands(const QString &cmds)
{
    m_resetCommands = cmds;
}

Utils::CommandLine GdbServerProvider::command() const
{
    return {};
}

bool GdbServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!IDebugServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const GdbServerProvider *>(&other);
    return m_startupMode == p->m_startupMode
            && m_initCommands == p->m_initCommands
            && m_resetCommands == p->m_resetCommands
            && m_useExtendedRemote == p->m_useExtendedRemote;
}

QVariantMap GdbServerProvider::toMap() const
{
    QVariantMap data = IDebugServerProvider::toMap();
    data.insert(startupModeKeyC, m_startupMode);
    data.insert(initCommandsKeyC, m_initCommands);
    data.insert(resetCommandsKeyC, m_resetCommands);
    data.insert(useExtendedRemoteKeyC, m_useExtendedRemote);
    return data;
}

bool GdbServerProvider::isValid() const
{
    return !channelString().isEmpty();
}

bool GdbServerProvider::aboutToRun(DebuggerRunTool *runTool,
                                   QString &errorMessage) const
{
    QTC_ASSERT(runTool, return false);
    const RunControl *runControl = runTool->runControl();
    const auto exeAspect = runControl->aspect<ExecutableAspect>();
    QTC_ASSERT(exeAspect, return false);

    const FilePath bin = exeAspect->executable();
    if (bin.isEmpty()) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Cannot debug: Local executable is not set.");
        return false;
    }
    if (!bin.exists()) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Cannot debug: Could not find executable for \"%1\".")
                .arg(bin.toString());
        return false;
    }

    Runnable inferior;
    inferior.executable = bin;
    if (const auto argAspect = runControl->aspect<ArgumentsAspect>())
        inferior.commandLineArguments = argAspect->arguments(runControl->macroExpander());
    runTool->setInferior(inferior);
    runTool->setSymbolFile(bin);
    runTool->setStartMode(AttachToRemoteServer);
    runTool->setCommandsAfterConnect(initCommands()); // .. and here?
    runTool->setCommandsForReset(resetCommands());
    runTool->setRemoteChannel(channelString());
    runTool->setUseContinueInsteadOfRun(true);
    runTool->setUseExtendedRemote(useExtendedRemote());
    return true;
}

RunWorker *GdbServerProvider::targetRunner(RunControl *runControl) const
{
    if (m_startupMode != GdbServerProvider::StartupOnNetwork)
        return nullptr;

    Runnable r;
    r.setCommandLine(command());
    // Command arguments are in host OS style as the bare metal's GDB servers are launched
    // on the host, not on that target.
    return new GdbServerProviderRunner(runControl, r);
}

bool GdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!IDebugServerProvider::fromMap(data))
        return false;

    m_startupMode = static_cast<StartupMode>(data.value(startupModeKeyC).toInt());
    m_initCommands = data.value(initCommandsKeyC).toString();
    m_resetCommands = data.value(resetCommandsKeyC).toString();
    m_useExtendedRemote = data.value(useExtendedRemoteKeyC).toBool();
    return true;
}

// GdbServerProviderConfigWidget

GdbServerProviderConfigWidget::GdbServerProviderConfigWidget(
        GdbServerProvider *provider)
    : IDebugServerProviderConfigWidget(provider)
{
    m_startupModeComboBox = new QComboBox(this);
    m_startupModeComboBox->setToolTip(tr("Choose the desired startup mode "
                                         "of the GDB server provider."));
    m_mainLayout->addRow(tr("Startup mode:"), m_startupModeComboBox);

    populateStartupModes();
    setFromProvider();

    connect(m_startupModeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GdbServerProviderConfigWidget::dirty);
}

void GdbServerProviderConfigWidget::apply()
{
    static_cast<GdbServerProvider *>(m_provider)->setStartupMode(startupMode());
    IDebugServerProviderConfigWidget::apply();
}

void GdbServerProviderConfigWidget::discard()
{
    setFromProvider();
    IDebugServerProviderConfigWidget::discard();
}

GdbServerProvider::StartupMode GdbServerProviderConfigWidget::startupModeFromIndex(
        int idx) const
{
    return static_cast<GdbServerProvider::StartupMode>(
                m_startupModeComboBox->itemData(idx).toInt());
}

GdbServerProvider::StartupMode GdbServerProviderConfigWidget::startupMode() const
{
    const int idx = m_startupModeComboBox->currentIndex();
    return startupModeFromIndex(idx);
}

void GdbServerProviderConfigWidget::setStartupMode(GdbServerProvider::StartupMode m)
{
    for (int idx = 0; idx < m_startupModeComboBox->count(); ++idx) {
        if (m == startupModeFromIndex(idx)) {
            m_startupModeComboBox->setCurrentIndex(idx);
            break;
        }
    }
}

static QString startupModeName(GdbServerProvider::StartupMode m)
{
    switch (m) {
    case GdbServerProvider::StartupOnNetwork:
        return GdbServerProviderConfigWidget::tr("Startup in TCP/IP Mode");
    case GdbServerProvider::StartupOnPipe:
        return GdbServerProviderConfigWidget::tr("Startup in Pipe Mode");
    default:
        return {};
    }
}

void GdbServerProviderConfigWidget::populateStartupModes()
{
    const QSet<GdbServerProvider::StartupMode> modes = static_cast<GdbServerProvider *>(
                m_provider)->supportedStartupModes();
    for (const auto mode : modes)
        m_startupModeComboBox->addItem(startupModeName(mode), mode);
}

void GdbServerProviderConfigWidget::setFromProvider()
{
    setStartupMode(static_cast<GdbServerProvider *>(m_provider)->startupMode());
}

QString GdbServerProviderConfigWidget::defaultInitCommandsTooltip()
{
    return QCoreApplication::translate("BareMetal",
                                       "Enter GDB commands to reset the board "
                                       "and to write the nonvolatile memory.");
}

QString GdbServerProviderConfigWidget::defaultResetCommandsTooltip()
{
    return QCoreApplication::translate("BareMetal",
                                       "Enter GDB commands to reset the hardware. "
                                       "The MCU should be halted after these commands.");
}

// GdbServerProviderRunner

GdbServerProviderRunner::GdbServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                                 const ProjectExplorer::Runnable &runnable)
    : SimpleTargetRunner(runControl)
{
    setId("BareMetalGdbServer");
    // Baremetal's GDB servers are launched on the host, not on the target.
    setStarter([this, runnable] { doStart(runnable, {}); });
}

} // namespace Internal
} // namespace BareMetal
