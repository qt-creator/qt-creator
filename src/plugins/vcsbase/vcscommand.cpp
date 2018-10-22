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

#include <coreplugin/documentmanager.h>
#include <coreplugin/vcsmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <utils/synchronousprocess.h>

#include <QProcessEnvironment>

using namespace Utils;

namespace VcsBase {

VcsCommand::VcsCommand(const QString &workingDirectory,
                       const QProcessEnvironment &environment) :
    Core::ShellCommand(workingDirectory, environment),
    m_preventRepositoryChanged(false)
{
    setOutputProxyFactory([this] {
        auto proxy = new OutputProxy;
        VcsOutputWindow *outputWindow = VcsOutputWindow::instance();

        connect(proxy, &OutputProxy::append,
                outputWindow, [](const QString &txt) { VcsOutputWindow::append(txt); });
        connect(proxy, &OutputProxy::appendSilently,
                outputWindow, &VcsOutputWindow::appendSilently);
        connect(proxy, &OutputProxy::appendError,
                outputWindow, &VcsOutputWindow::appendError);
        connect(proxy, &OutputProxy::appendCommand,
                outputWindow, &VcsOutputWindow::appendCommand);
        connect(proxy, &OutputProxy::appendMessage,
                outputWindow, &VcsOutputWindow::appendMessage);

        return proxy;
    });
    connect(this, &VcsCommand::started, this, [this] {
        if (flags() & ExpectRepoChanges) {
            Core::DocumentManager::setAutoReloadPostponed(true);
            CppTools::CppModelManager::instance()->setBackendJobsPostponed(true);
        }
    });
    connect(this, &VcsCommand::finished, this, [this] {
        if (flags() & ExpectRepoChanges) {
            Core::DocumentManager::setAutoReloadPostponed(false);
            CppTools::CppModelManager::instance()->setBackendJobsPostponed(false);
        }
    });
}

const QProcessEnvironment VcsCommand::processEnvironment() const
{
    QProcessEnvironment env = Core::ShellCommand::processEnvironment();
    VcsBasePlugin::setProcessEnvironment(&env, flags() & ForceCLocale, VcsBasePlugin::sshPrompt());
    return env;
}

SynchronousProcessResponse VcsCommand::runCommand(const FileName &binary,
                                                         const QStringList &arguments, int timeoutS,
                                                         const QString &workingDirectory,
                                                         const ExitCodeInterpreter &interpreter)
{
    SynchronousProcessResponse response
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
        processFlags |= SynchronousProcess::UnixTerminalDisabled;
    return processFlags;
}

void VcsCommand::coreAboutToClose()
{
    m_preventRepositoryChanged = true;
    abort();
}

} // namespace VcsBase
