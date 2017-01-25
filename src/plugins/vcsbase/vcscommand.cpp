/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "vcscommand.h"
#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"

#include <coreplugin/vcsmanager.h>
#include <utils/synchronousprocess.h>

#include <QProcessEnvironment>

namespace VcsBase {

VcsCommand::VcsCommand(const QString &workingDirectory,
                       const QProcessEnvironment &environment) :
    Core::ShellCommand(workingDirectory, environment),
    m_preventRepositoryChanged(false)
{
    setOutputProxyFactory([this]() -> Utils::OutputProxy * {
        auto proxy = new Utils::OutputProxy;
        VcsOutputWindow *outputWindow = VcsOutputWindow::instance();

        connect(proxy, &Utils::OutputProxy::append,
                outputWindow, [](const QString &txt) { VcsOutputWindow::append(txt); });
        connect(proxy, &Utils::OutputProxy::appendSilently,
                outputWindow, &VcsOutputWindow::appendSilently);
        connect(proxy, &Utils::OutputProxy::appendError,
                outputWindow, &VcsOutputWindow::appendError);
        connect(proxy, &Utils::OutputProxy::appendCommand,
                outputWindow, &VcsOutputWindow::appendCommand);
        connect(proxy, &Utils::OutputProxy::appendMessage,
                outputWindow, &VcsOutputWindow::appendMessage);

        return proxy;
    });
}

const QProcessEnvironment VcsCommand::processEnvironment() const
{
    QProcessEnvironment env = Core::ShellCommand::processEnvironment();
    VcsBasePlugin::setProcessEnvironment(&env, flags() & ForceCLocale, VcsBasePlugin::sshPrompt());
    return env;
}

Utils::SynchronousProcessResponse VcsCommand::runCommand(const Utils::FileName &binary,
                                                         const QStringList &arguments, int timeoutS,
                                                         const QString &workingDirectory,
                                                         const Utils::ExitCodeInterpreter &interpreter)
{
    Utils::SynchronousProcessResponse response
            = Core::ShellCommand::runCommand(binary, arguments, timeoutS, workingDirectory,
                                             interpreter);
    emitRepositoryChanged(workingDirectory);
    return response;
}

void VcsCommand::emitRepositoryChanged(const QString &workingDirectory)
{
    if (m_preventRepositoryChanged || !(flags() & VcsCommand::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    Core::VcsManager::emitRepositoryChanged(workDirectory(workingDirectory));
}

unsigned VcsCommand::processFlags() const
{
    unsigned processFlags = 0;
    if (!VcsBasePlugin::sshPrompt().isEmpty() && (flags() & SshPasswordPrompt))
        processFlags |= Utils::SynchronousProcess::UnixTerminalDisabled;
    return processFlags;
}

void VcsCommand::coreAboutToClose()
{
    m_preventRepositoryChanged = true;
    abort();
}

} // namespace VcsBase
