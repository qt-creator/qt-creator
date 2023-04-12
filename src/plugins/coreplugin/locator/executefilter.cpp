// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "executefilter.h"

#include "../coreplugintr.h"
#include "../icore.h"
#include "../editormanager/editormanager.h"
#include "../messagemanager.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

using namespace Utils;

namespace Core::Internal {

ExecuteFilter::ExecuteFilter()
{
    setId("Execute custom commands");
    setDisplayName(Tr::tr("Execute Custom Commands"));
    setDescription(Tr::tr(
        "Runs an arbitrary command with arguments. The command is searched for in the PATH "
        "environment variable if needed. Note that the command is run directly, not in a shell."));
    setDefaultShortcutString("!");
    setPriority(High);
    setDefaultIncludedByDefault(false);
}

ExecuteFilter::~ExecuteFilter()
{
    removeProcess();
}

void ExecuteFilter::acceptCommand(const QString &cmd)
{
    const QString displayName = cmd.trimmed();
    const int index = m_commandHistory.indexOf(displayName);
    if (index != -1 && index != 0)
        m_commandHistory.removeAt(index);
    if (index != 0)
        m_commandHistory.prepend(displayName);
    static const int maxHistory = 100;
    while (m_commandHistory.size() > maxHistory)
        m_commandHistory.removeLast();

    bool found;
    QString workingDirectory = globalMacroExpander()->value("CurrentDocument:Path", &found);
    if (!found || workingDirectory.isEmpty())
        workingDirectory = globalMacroExpander()->value("CurrentDocument:Project:Path", &found);

    const ExecuteData data{CommandLine::fromUserInput(displayName, globalMacroExpander()),
                           FilePath::fromString(workingDirectory)};
    if (m_process) {
        const QString info(Tr::tr("Previous command is still running (\"%1\").\n"
                                  "Do you want to kill it?").arg(headCommand()));
        const auto result = QMessageBox::question(ICore::dialogParent(),
                                                  Tr::tr("Kill Previous Process?"), info,
                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                  QMessageBox::Yes);
        if (result == QMessageBox::Cancel)
            return;
        if (result == QMessageBox::No) {
            m_taskQueue.enqueue(data);
            return;
        }
        removeProcess();
    }
    m_taskQueue.enqueue(data);
    runHeadCommand();
}

QList<LocatorFilterEntry> ExecuteFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                    const QString &entry)
{
    QList<LocatorFilterEntry> results;
    if (!entry.isEmpty()) { // avoid empty entry
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = entry;
        filterEntry.acceptor = [this, entry] { acceptCommand(entry); return AcceptResult(); };
        results.append(filterEntry);
    }
    QList<LocatorFilterEntry> others;
    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);
    for (const QString &cmd : std::as_const(m_commandHistory)) {
        if (future.isCanceled())
            break;
        if (cmd == entry) // avoid repeated entry
            continue;
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = cmd;
        filterEntry.acceptor = [this, cmd] { acceptCommand(cmd); return AcceptResult(); };
        const int index = cmd.indexOf(entry, 0, entryCaseSensitivity);
        if (index >= 0) {
            filterEntry.highlightInfo = {index, int(entry.length())};
            results.append(filterEntry);
        } else {
            others.append(filterEntry);
        }
    }
    results.append(others);
    return results;
}

void ExecuteFilter::done()
{
    QTC_ASSERT(m_process, return);
    MessageManager::writeFlashing(m_process->exitMessage());
    EditorManager::updateWindowTitles(); // Refresh VCS topic if needed

    removeProcess();
    runHeadCommand();
}

void ExecuteFilter::readStandardOutput()
{
    QTC_ASSERT(m_process, return);
    const QByteArray data = m_process->readAllRawStandardOutput();
    MessageManager::writeSilently(
        QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(), &m_stdoutState));
}

void ExecuteFilter::readStandardError()
{
    QTC_ASSERT(m_process, return);
    const QByteArray data = m_process->readAllRawStandardError();
    MessageManager::writeSilently(
        QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(), &m_stderrState));
}

void ExecuteFilter::runHeadCommand()
{
    if (!m_taskQueue.isEmpty()) {
        const ExecuteData &d = m_taskQueue.head();
        if (d.command.executable().isEmpty()) {
            MessageManager::writeDisrupting(
                Tr::tr("Could not find executable for \"%1\".").arg(d.command.executable().toUserOutput()));
            m_taskQueue.dequeue();
            runHeadCommand();
            return;
        }
        MessageManager::writeDisrupting(Tr::tr("Starting command \"%1\".").arg(headCommand()));
        QTC_CHECK(!m_process);
        createProcess();
        m_process->setWorkingDirectory(d.workingDirectory);
        m_process->setCommand(d.command);
        m_process->start();
    }
}

void ExecuteFilter::createProcess()
{
    if (m_process)
        return;

    m_process = new QtcProcess;
    m_process->setEnvironment(Environment::systemEnvironment());
    connect(m_process, &QtcProcess::done, this, &ExecuteFilter::done);
    connect(m_process, &QtcProcess::readyReadStandardOutput, this, &ExecuteFilter::readStandardOutput);
    connect(m_process, &QtcProcess::readyReadStandardError, this, &ExecuteFilter::readStandardError);
}

void ExecuteFilter::removeProcess()
{
    if (!m_process)
        return;

    m_taskQueue.dequeue();
    m_process->deleteLater();
    m_process = nullptr;
}

const char historyKey[] = "history";

void ExecuteFilter::saveState(QJsonObject &object) const
{
    if (!m_commandHistory.isEmpty())
        object.insert(historyKey, QJsonArray::fromStringList(m_commandHistory));
}

void ExecuteFilter::restoreState(const QJsonObject &object)
{
    m_commandHistory = Utils::transform(object.value(historyKey).toArray().toVariantList(),
                                        &QVariant::toString);
}

QString ExecuteFilter::headCommand() const
{
    if (m_taskQueue.isEmpty())
        return QString();
    const ExecuteData &data = m_taskQueue.head();
    return data.command.toUserOutput();
}

} // Core::Internal
