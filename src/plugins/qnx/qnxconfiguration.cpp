/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxconfiguration.h"
#include "qnxutils.h"

#include "qnxtoolchain.h"
#include "debugger/debuggeritem.h"

#include <projectexplorer/toolchainmanager.h>

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QFileInfo>

namespace Qnx {
namespace Internal {
QnxConfiguration::QnxConfiguration(const Utils::FileName &sdpEnvFile)
    : QnxBaseConfiguration(sdpEnvFile)
{
}

bool QnxConfiguration::activate()
{
    if (isActive())
        return true;

    if (!isValid()) {
        QString errorMessage = tr("The following errors occurred while activating the QNX configuration:");
        foreach (const QString &error, validationErrors())
            errorMessage += QLatin1String("\n") + error;

        QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Set Up QNX Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    // Create and register toolchain
    createToolChain(QnxArchitecture::ArmLeV7,
                    tr("QCC for QNX armlev7"),
                    sdpPath().toString());
    createToolChain(QnxArchitecture::X86,
                    tr("QCC for QNX x86"),
                    sdpPath().toString());

    // Create and register debuggers
    createDebuggerItem(QnxArchitecture::ArmLeV7,
                       tr("Debugger for QNX armlev7"));

    createDebuggerItem(QnxArchitecture::X86,
                       tr("Debugger for QNX x86"));

    return true;
}

void QnxConfiguration::deactivate()
{
    if (!isActive())
        return;

    foreach (ProjectExplorer::ToolChain *tc,
             ProjectExplorer::ToolChainManager::toolChains()) {
        if (tc->compilerCommand() == qccCompilerPath())
            ProjectExplorer::ToolChainManager::deregisterToolChain(tc);
    }

    foreach (Debugger::DebuggerItem debuggerItem,
             Debugger::DebuggerItemManager::debuggers()) {
        if (debuggerItem.command() == armDebuggerPath() ||
                debuggerItem.command() == x86DebuggerPath())
            Debugger::DebuggerItemManager::
                    deregisterDebugger(debuggerItem.id());
    }
}

bool QnxConfiguration::isActive() const
{
    bool hasToolChain = false;
    bool hasDebugger = false;
    foreach (ProjectExplorer::ToolChain *tc,
             ProjectExplorer::ToolChainManager::toolChains()) {
        if (tc->compilerCommand() == qccCompilerPath()) {
            hasToolChain = true;
            break;
        }
    }

    foreach (Debugger::DebuggerItem debuggerItem,
             Debugger::DebuggerItemManager::debuggers()) {
        if (debuggerItem.command() == armDebuggerPath() ||
                debuggerItem.command() == x86DebuggerPath()) {
            hasDebugger = true;
            break;
        }
    }

    return hasToolChain && hasDebugger;
}

Utils::FileName QnxConfiguration::sdpPath() const
{
    return envFile().parentDir();
}

}
}
