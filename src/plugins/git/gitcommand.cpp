/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "gitcommand.h"
#include "gitconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>

namespace Git {
namespace Internal {

// Convert environment to list, default to system one.
static inline QStringList environmentToList(const ProjectExplorer::Environment &environment)
{
    const QStringList list = environment.toStringList();
    if (!list.empty())
        return list;
    return ProjectExplorer::Environment::systemEnvironment().toStringList();
}

GitCommand::Job::Job(const QStringList &a, int t) :
    arguments(a),
    timeout(t)
{
}

GitCommand::GitCommand(const QString &workingDirectory,
                        ProjectExplorer::Environment &environment)  :
    m_workingDirectory(workingDirectory),
    m_environment(environmentToList(environment))
{
}

void GitCommand::addJob(const QStringList &arguments, int timeout)
{
    m_jobs.push_back(Job(arguments, timeout));
}

void GitCommand::execute()
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::execute" << m_workingDirectory << m_jobs.size();

    if (m_jobs.empty())
        return;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(this, &GitCommand::run);
    const QString taskName = QLatin1String("Git ") + m_jobs.front().arguments.at(0);

    Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                     QLatin1String("Git.action"),
                                     Core::ProgressManager::CloseOnSuccess);
}

void GitCommand::run()
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::run" << m_workingDirectory << m_jobs.size();
    QProcess process;
    if (!m_workingDirectory.isEmpty())
        process.setWorkingDirectory(m_workingDirectory);

    process.setEnvironment(m_environment);

    QByteArray output;
    QString error;

    const int count = m_jobs.size();
    bool ok = true;
    for (int j = 0; j < count; j++) {
        if (Git::Constants::debug)
            qDebug() << "GitCommand::run" << j << '/' << count << m_jobs.at(j).arguments;

        process.start(QLatin1String(Constants::GIT_BINARY), m_jobs.at(j).arguments);
        if (!process.waitForFinished(m_jobs.at(j).timeout * 1000)) {
            ok = false;
            error += QLatin1String("Error: Git timed out");
            break;
        }

        output += process.readAllStandardOutput();
        error += QString::fromLocal8Bit(process.readAllStandardError());
    }

    // Special hack: Always produce output for diff
    if (ok && output.isEmpty() && m_jobs.front().arguments.at(0) == QLatin1String("diff")) {
        output += "The file does not differ from HEAD";
    }

    if (ok && !output.isEmpty())
        emit outputData(output);

    if (!error.isEmpty())
        emit errorText(error);

    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

} // namespace Internal
} // namespace Git
