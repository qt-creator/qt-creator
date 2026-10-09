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
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>

using namespace QtTaskTree;
using namespace Utils;

namespace Core::Internal {

static const int kMaxHistory = 100;

ExecuteFilter::ExecuteFilter()
{
    setId("Execute custom commands");
    setDisplayName(Tr::tr("Execute Custom Commands"));
    setDescription(
        Tr::tr(
            "Runs an arbitrary command with arguments. The command is searched for in the PATH "
            "environment variable if needed."));
    setDefaultShortcutString("!");
    setPriority(High);
}

ExecuteFilter::~ExecuteFilter()
{
    removeProcess();
}

LocatorMatcherTasks ExecuteFilter::matchers()
{
    const auto onSetup = [this] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        const QString input = storage.input();
        LocatorFilterEntries entries;
        if (!input.isEmpty()) { // avoid empty entry
            LocatorFilterEntry entry;
            entry.displayName = input;
            entry.acceptor = [this, input] { acceptCommand(input); return AcceptResult(); };
            entries.append(entry);
        }
        LocatorFilterEntries others;
        const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(input);
        for (const QString &cmd : std::as_const(m_commandHistory)) {
            if (cmd == input) // avoid repeated entry
                continue;
            LocatorFilterEntry entry;
            entry.displayName = cmd;
            entry.acceptor = [this, cmd] { acceptCommand(cmd); return AcceptResult(); };
            const int index = cmd.indexOf(input, 0, entryCaseSensitivity);
            if (index >= 0) {
                entry.highlightInfo = {index, int(input.size())};
                entries.append(entry);
            } else {
                others.append(entry);
            }
        }
        storage.reportOutput(entries + others);
    };
    return {QSyncTask(onSetup)};
}

void ExecuteFilter::acceptCommand(const QString &cmd)
{
    const QString displayName = cmd.trimmed();
    const int index = m_commandHistory.indexOf(displayName);
    if (index != -1 && index != 0)
        m_commandHistory.removeAt(index);
    if (index != 0)
        m_commandHistory.prepend(displayName);
    while (m_commandHistory.size() > kMaxHistory)
        m_commandHistory.removeLast();

    bool found;
    QString workingDirectory = globalMacroExpander()->value("CurrentDocument:Path", &found);
    if (!found || workingDirectory.isEmpty())
        workingDirectory = globalMacroExpander()->value("CurrentDocument:Project:Path", &found);

    const ExecuteData data{CommandLine::fromUserInput(displayName, globalMacroExpander()),
                           FilePath::fromString(workingDirectory)};
    if (m_process) {
        const QString info
            = m_taskQueue.size() > 1
                  ? Tr::tr(
                        "Previous command is still running (\"%1\") and %n more queued.\n"
                        "Do you want to kill the running command and clear the queue, or queue the "
                        "new command?",
                        nullptr,
                        m_taskQueue.size() - 1)
                        .arg(headCommand())
                  : Tr::tr(
                        "Previous command is still running (\"%1\").\n"
                        "Do you want to kill it or queue the new command?")
                        .arg(headCommand());
        QMessageBox
            box(QMessageBox::Question,
                Tr::tr("Command Still Running"),
                info,
                QMessageBox::Cancel,
                ICore::dialogParent());
        //: kill the running command
        QPushButton *killButton = box.addButton(Tr::tr("Kill"), QMessageBox::DestructiveRole);
        //: queue the new command
        QPushButton *queueButton = box.addButton(Tr::tr("Queue"), QMessageBox::AcceptRole);
        box.setDefaultButton(queueButton);
        box.exec();
        if (box.clickedButton() == queueButton) {
            m_taskQueue.append(data);
            if (!m_process && m_taskQueue.size() == 1) {
                // previous command(s) have finished in the meantime
                runHeadCommand();
            }
            return;
        }
        if (box.clickedButton() != killButton)
            return;
        if (!m_taskQueue.isEmpty())
            MessageManager::writeFlashing(Tr::tr("Killing command \"%1\".").arg(headCommand()));
        removeProcess();
        m_taskQueue.clear();
    }
    m_taskQueue.append(data);
    runHeadCommand();
}

void ExecuteFilter::done()
{
    QTC_ASSERT(m_process, return);
    MessageManager::writeFlashing(m_process->exitMessage());
    EditorManager::updateWindowTitles(); // Refresh version control topic if needed

    removeProcess();
    runHeadCommand();
}

void ExecuteFilter::readStdOutput()
{
    QTC_ASSERT(m_process, return);
    MessageManager::writeSilently(m_process->readAllStandardOutput());
}

void ExecuteFilter::readStdError()
{
    QTC_ASSERT(m_process, return);
    MessageManager::writeSilently(m_process->readAllStandardError());
}

void ExecuteFilter::runHeadCommand()
{
    if (!m_taskQueue.isEmpty()) {
        const ExecuteData &d = m_taskQueue.first();
        if (d.command.executable().isEmpty()) {
            MessageManager::writeDisrupting(Tr::tr("Could not find executable for \"%1\".")
                                                .arg(d.command.executable().toUserOutput()));
            m_taskQueue.removeFirst();
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

    m_process = new Process;
    m_process->setEnvironment(Environment::systemEnvironment());
    connect(m_process, &Process::done, this, &ExecuteFilter::done);
    connect(m_process, &Process::readyReadStandardOutput, this, &ExecuteFilter::readStdOutput);
    connect(m_process, &Process::readyReadStandardError, this, &ExecuteFilter::readStdError);
}

void ExecuteFilter::removeProcess()
{
    if (!m_process)
        return;

    m_taskQueue.removeFirst();
    m_process->disconnect(this);
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
    QVariantList history = object.value(historyKey).toArray().toVariantList();
    if (history.size() > kMaxHistory)
        history.resize(kMaxHistory);
    m_commandHistory = Utils::transform(history, &QVariant::toString);
}

QString ExecuteFilter::headCommand() const
{
    if (m_taskQueue.isEmpty())
        return QString();
    const ExecuteData &data = m_taskQueue.first();
    return data.command.toUserOutput();
}

} // namespace Core::Internal
