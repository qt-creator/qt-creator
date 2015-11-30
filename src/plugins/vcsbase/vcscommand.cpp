/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "vcscommand.h"
#include "vcsbaseplugin.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <vcsbase/vcsoutputwindow.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>
#include <QVariant>
#include <QStringList>
#include <QTextCodec>
#include <QMutex>

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
                                      outputWindow, [](const QString &txt) { VcsOutputWindow::append(txt); },
                                      Qt::QueuedConnection);
                              connect(proxy, &Utils::OutputProxy::appendSilently,
                                      outputWindow, &VcsOutputWindow::appendSilently,
                                      Qt::QueuedConnection);
                              connect(proxy, &Utils::OutputProxy::appendError,
                                      outputWindow, &VcsOutputWindow::appendError,
                                      Qt::QueuedConnection);
                              connect(proxy, &Utils::OutputProxy::appendCommand,
                                      outputWindow, &VcsOutputWindow::appendCommand,
                                      Qt::QueuedConnection);
                              connect(proxy, &Utils::OutputProxy::appendMessage,
                                      outputWindow, &VcsOutputWindow::appendMessage,
                                      Qt::QueuedConnection);

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
                                                         Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response
            = Core::ShellCommand::runCommand(binary, arguments, timeoutS, workingDirectory,
                                             interpreter);
    emitRepositoryChanged(workingDirectory);
    return response;
}

bool VcsCommand::runFullySynchronous(const Utils::FileName &binary, const QStringList &arguments,
                                     int timeoutS, QByteArray *outputData, QByteArray *errorData,
                                     const QString &workingDirectory)
{
    bool result = Core::ShellCommand::runFullySynchronous(binary, arguments, timeoutS,
                                                          outputData, errorData, workingDirectory);
    emitRepositoryChanged(workingDirectory);
    return result;
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
