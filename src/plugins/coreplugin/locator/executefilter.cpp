// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "executefilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

using namespace Core;
using namespace Core::Internal;

using namespace Utils;

ExecuteFilter::ExecuteFilter()
{
    setId("Execute custom commands");
    setDisplayName(tr("Execute Custom Commands"));
    setDescription(tr(
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

QList<LocatorFilterEntry> ExecuteFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                             const QString &entry)
{
    QList<LocatorFilterEntry> value;
    if (!entry.isEmpty()) // avoid empty entry
        value.append(LocatorFilterEntry(this, entry, QVariant()));
    QList<LocatorFilterEntry> others;
    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);
    for (const QString &cmd : std::as_const(m_commandHistory)) {
        if (future.isCanceled())
            break;
        if (cmd == entry) // avoid repeated entry
            continue;
        LocatorFilterEntry filterEntry(this, cmd, QVariant());
        const int index = cmd.indexOf(entry, 0, entryCaseSensitivity);
        if (index >= 0) {
            filterEntry.highlightInfo = {index, int(entry.length())};
            value.append(filterEntry);
        } else {
            others.append(filterEntry);
        }
    }
    value.append(others);
    return value;
}

void ExecuteFilter::accept(const LocatorFilterEntry &selection,
                           QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    auto p = const_cast<ExecuteFilter *>(this);

    const QString value = selection.displayName.trimmed();

    const int index = m_commandHistory.indexOf(value);
    if (index != -1 && index != 0)
        p->m_commandHistory.removeAt(index);
    if (index != 0)
        p->m_commandHistory.prepend(value);
    static const int maxHistory = 100;
    while (p->m_commandHistory.size() > maxHistory)
        p->m_commandHistory.removeLast();

    bool found;
    QString workingDirectory = Utils::globalMacroExpander()->value("CurrentDocument:Path", &found);
    if (!found || workingDirectory.isEmpty())
        workingDirectory = Utils::globalMacroExpander()->value("CurrentDocument:Project:Path", &found);

    ExecuteData d;
    d.command = CommandLine::fromUserInput(value, Utils::globalMacroExpander());
    d.workingDirectory = FilePath::fromString(workingDirectory);

    if (m_process) {
        const QString info(tr("Previous command is still running (\"%1\").\nDo you want to kill it?")
                           .arg(p->headCommand()));
        int r = QMessageBox::question(ICore::dialogParent(), tr("Kill Previous Process?"), info,
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                      QMessageBox::Yes);
        if (r == QMessageBox::Cancel)
            return;
        if (r == QMessageBox::No) {
            p->m_taskQueue.enqueue(d);
            return;
        }
        p->removeProcess();
    }

    p->m_taskQueue.enqueue(d);
    p->runHeadCommand();
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
    const QByteArray data = m_process->readAllStandardOutput();
    MessageManager::writeSilently(
        QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(), &m_stdoutState));
}

void ExecuteFilter::readStandardError()
{
    QTC_ASSERT(m_process, return);
    const QByteArray data = m_process->readAllStandardError();
    MessageManager::writeSilently(
        QTextCodec::codecForLocale()->toUnicode(data.constData(), data.size(), &m_stderrState));
}

void ExecuteFilter::runHeadCommand()
{
    if (!m_taskQueue.isEmpty()) {
        const ExecuteData &d = m_taskQueue.head();
        if (d.command.executable().isEmpty()) {
            MessageManager::writeDisrupting(
                tr("Could not find executable for \"%1\".").arg(d.command.executable().toUserOutput()));
            m_taskQueue.dequeue();
            runHeadCommand();
            return;
        }
        MessageManager::writeDisrupting(tr("Starting command \"%1\".").arg(headCommand()));
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

    m_process = new Utils::QtcProcess;
    m_process->setEnvironment(Utils::Environment::systemEnvironment());
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
