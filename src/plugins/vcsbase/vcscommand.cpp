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
#include "vcsplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace VcsBase {

VcsCommand::VcsCommand(const QString &workingDirectory, const Environment &environment) :
    Core::ShellCommand(workingDirectory, environment),
    m_preventRepositoryChanged(false)
{
    VcsOutputWindow::setRepository(workingDirectory);
    setDisableUnixTerminal();
    m_sshPrompt = VcsBase::sshPrompt();

    connect(this, &VcsCommand::started, this, [this] {
        if (flags() & ExpectRepoChanges)
            Utils::GlobalFileChangeBlocker::instance()->forceBlocked(true);
    });
    connect(this, &VcsCommand::finished, this, [this] {
        if (flags() & ExpectRepoChanges)
            Utils::GlobalFileChangeBlocker::instance()->forceBlocked(false);
    });

    VcsOutputWindow *outputWindow = VcsOutputWindow::instance();
    connect(this, &ShellCommand::append, outputWindow, [outputWindow](const QString &t) {
        outputWindow->append(t);
    });
    connect(this, &ShellCommand::appendSilently, outputWindow, &VcsOutputWindow::appendSilently);
    connect(this, &ShellCommand::appendError, outputWindow, &VcsOutputWindow::appendError);
    connect(this, &ShellCommand::appendCommand, outputWindow, &VcsOutputWindow::appendCommand);
    connect(this, &ShellCommand::appendMessage, outputWindow, &VcsOutputWindow::appendMessage);
}

const Environment VcsCommand::processEnvironment() const
{
    Environment env = Core::ShellCommand::processEnvironment();
    VcsBase::setProcessEnvironment(&env, flags() & ForceCLocale, m_sshPrompt);
    return env;
}

void VcsCommand::runCommand(QtcProcess &proc,
                            const CommandLine &command,
                            const QString &workingDirectory)
{
    ShellCommand::runCommand(proc, command, workingDirectory);
    emitRepositoryChanged(workingDirectory);
}

void VcsCommand::addTask(QFuture<void> &future)
{
    Core::ShellCommand::addTask(future);
    Internal::VcsPlugin::addFuture(future);
}

void VcsCommand::emitRepositoryChanged(const QString &workingDirectory)
{
    if (m_preventRepositoryChanged || !(flags() & VcsCommand::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    Core::VcsManager::emitRepositoryChanged(workDirectory(workingDirectory));
}

void VcsCommand::coreAboutToClose()
{
    m_preventRepositoryChanged = true;
    abort();
}

} // namespace VcsBase
