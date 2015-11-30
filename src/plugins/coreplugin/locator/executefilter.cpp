/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "executefilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/macroexpander.h>

#include <QMessageBox>

using namespace Core;
using namespace Core::Internal;

ExecuteFilter::ExecuteFilter()
{
    setId("Execute custom commands");
    setDisplayName(tr("Execute Custom Commands"));
    setShortcutString(QString(QLatin1Char('!')));
    setPriority(High);
    setIncludedByDefault(false);

    m_process = new Utils::QtcProcess(this);
    m_process->setEnvironment(Utils::Environment::systemEnvironment());
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this,
            SLOT(finished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));

    m_runTimer.setSingleShot(true);
    connect(&m_runTimer, SIGNAL(timeout()), this, SLOT(runHeadCommand()));
}

QList<LocatorFilterEntry> ExecuteFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                             const QString &entry)
{
    QList<LocatorFilterEntry> value;
    if (!entry.isEmpty()) // avoid empty entry
        value.append(LocatorFilterEntry(this, entry, QVariant()));
    QList<LocatorFilterEntry> others;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    foreach (const QString &i, m_commandHistory) {
        if (future.isCanceled())
            break;
        if (i == entry) // avoid repeated entry
            continue;
        if (i.startsWith(entry, caseSensitivityForPrefix))
            value.append(LocatorFilterEntry(this, i, QVariant()));
        else
            others.append(LocatorFilterEntry(this, i, QVariant()));
    }
    value.append(others);
    return value;
}

void ExecuteFilter::accept(LocatorFilterEntry selection) const
{
    ExecuteFilter *p = const_cast<ExecuteFilter *>(this);

    const QString value = selection.displayName.trimmed();
    const int index = m_commandHistory.indexOf(value);
    if (index != -1 && index != 0)
        p->m_commandHistory.removeAt(index);
    if (index != 0)
        p->m_commandHistory.prepend(value);

    bool found;
    QString workingDirectory = Utils::globalMacroExpander()->value("CurrentDocument:Path", &found);
    if (!found || workingDirectory.isEmpty())
        workingDirectory = Utils::globalMacroExpander()->value("CurrentProject:Path", &found);

    ExecuteData d;
    d.workingDirectory = workingDirectory;
    const int pos = value.indexOf(QLatin1Char(' '));
    if (pos == -1) {
        d.executable = value;
    } else {
        d.executable = value.left(pos);
        d.arguments = value.right(value.length() - pos - 1);
    }

    if (m_process->state() != QProcess::NotRunning) {
        const QString info(tr("Previous command is still running (\"%1\").\nDo you want to kill it?")
                           .arg(p->headCommand()));
        int r = QMessageBox::question(ICore::dialogParent(), tr("Kill Previous Process?"), info,
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                      QMessageBox::Yes);
        if (r == QMessageBox::Yes)
            m_process->kill();
        if (r != QMessageBox::Cancel)
            p->m_taskQueue.enqueue(d);
        return;
    }

    p->m_taskQueue.enqueue(d);
    p->runHeadCommand();
}

void ExecuteFilter::finished(int exitCode, QProcess::ExitStatus status)
{
    const QString commandName = headCommand();
    QString message;
    if (status == QProcess::NormalExit && exitCode == 0)
        message = tr("Command \"%1\" finished.").arg(commandName);
    else
        message = tr("Command \"%1\" failed.").arg(commandName);
    MessageManager::write(message);

    m_taskQueue.dequeue();
    if (!m_taskQueue.isEmpty())
        m_runTimer.start(500);
}

void ExecuteFilter::readStandardOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    MessageManager::write(QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(),
                                                                  &m_stdoutState));
}

void ExecuteFilter::readStandardError()
{
    static QTextCodec::ConverterState state;
    QByteArray data = m_process->readAllStandardError();
    MessageManager::write(QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(),
                                                                  &m_stderrState));
}

void ExecuteFilter::runHeadCommand()
{
    if (!m_taskQueue.isEmpty()) {
        const ExecuteData &d = m_taskQueue.head();
        const Utils::FileName fullPath = Utils::Environment::systemEnvironment().searchInPath(d.executable);
        if (fullPath.isEmpty()) {
            MessageManager::write(tr("Could not find executable for \"%1\".").arg(d.executable));
            m_taskQueue.dequeue();
            runHeadCommand();
            return;
        }
        MessageManager::write(tr("Starting command \"%1\".").arg(headCommand()));
        m_process->setWorkingDirectory(d.workingDirectory);
        m_process->setCommand(fullPath.toString(), d.arguments);
        m_process->start();
        m_process->closeWriteChannel();
        if (!m_process->waitForStarted(1000)) {
             MessageManager::write(tr("Could not start process: %1.").arg(m_process->errorString()));
             m_taskQueue.dequeue();
             runHeadCommand();
        }
    }
}

QString ExecuteFilter::headCommand() const
{
    if (m_taskQueue.isEmpty())
        return QString();
    const ExecuteData &data = m_taskQueue.head();
    if (data.arguments.isEmpty())
        return data.executable;
    else
        return data.executable + QLatin1Char(' ') + data.arguments;
}
