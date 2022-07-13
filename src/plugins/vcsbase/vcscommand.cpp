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

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>

#include <utils/environment.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/qtcprocess.h>

#include <QFutureWatcher>

using namespace Core;
using namespace Utils;

namespace VcsBase {

VcsCommand::VcsCommand(const FilePath &workingDirectory, const Environment &environment) :
    ShellCommand(workingDirectory, environment),
    m_preventRepositoryChanged(false)
{
    Environment env = environment;
    VcsBase::setProcessEnvironment(&env);
    setEnvironment(env);

    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this] {
        m_preventRepositoryChanged = true;
        abort();
    });

    VcsOutputWindow::setRepository(workingDirectory.toString());
    setDisableUnixTerminal();

    connect(this, &ShellCommand::started, this, [this] {
        if (flags() & ExpectRepoChanges)
            GlobalFileChangeBlocker::instance()->forceBlocked(true);
    });
    connect(this, &ShellCommand::finished, this, [this] {
        if (flags() & ExpectRepoChanges)
            GlobalFileChangeBlocker::instance()->forceBlocked(false);
    });

    VcsOutputWindow *outputWindow = VcsOutputWindow::instance();
    connect(this, &ShellCommand::append, outputWindow, [outputWindow](const QString &t) {
        outputWindow->append(t);
    });
    connect(this, &ShellCommand::appendSilently, outputWindow, &VcsOutputWindow::appendSilently);
    connect(this, &ShellCommand::appendError, outputWindow, &VcsOutputWindow::appendError);
    connect(this, &ShellCommand::appendCommand, outputWindow, &VcsOutputWindow::appendCommand);
    connect(this, &ShellCommand::appendMessage, outputWindow, &VcsOutputWindow::appendMessage);

    connect(this, &ShellCommand::executedAsync, this, &VcsCommand::addTask);
    connect(this, &ShellCommand::runCommandFinished, this, &VcsCommand::postRunCommand);
}

void VcsCommand::addTask(const QFuture<void> &future)
{
    if ((flags() & SuppressCommandLogging))
        return;

    const QString name = displayName();
    const auto id = Id::fromString(name + QLatin1String(".action"));
    if (hasProgressParser()) {
        ProgressManager::addTask(future, name, id);
    } else {
        // add a timed tasked based on timeout
        // we cannot access the future interface directly, so we need to create a new one
        // with the same lifetime
        auto fi = new QFutureInterface<void>();
        auto watcher = new QFutureWatcher<void>();
        connect(watcher, &QFutureWatcherBase::finished, [fi, watcher] {
            fi->reportFinished();
            delete fi;
            watcher->deleteLater();
        });
        watcher->setFuture(future);
        ProgressManager::addTimedTask(*fi, name, id, qMax(2, timeoutS() / 5)/*itsmagic*/);
    }

    Internal::VcsPlugin::addFuture(future);
}

void VcsCommand::postRunCommand(const FilePath &workingDirectory)
{
    if (m_preventRepositoryChanged || !(flags() & ShellCommand::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    VcsManager::emitRepositoryChanged(workingDirectory);
}

} // namespace VcsBase
