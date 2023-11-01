// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processprogress.h"

#include "progressmanager.h"
#include "../coreplugintr.h"

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QFutureWatcher>
#include <QPointer>

using namespace Utils;

namespace Core {

class ProcessProgressPrivate : public QObject
{
public:
    explicit ProcessProgressPrivate(ProcessProgress *progress, Process *process);
    ~ProcessProgressPrivate();

    QString displayName() const;
    void parseProgress(const QString &inputText);

    Process *m_process = nullptr;
    ProgressParser m_parser = {};
    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> m_futureInterface;
    QPointer<FutureProgress> m_futureProgress;
    QString m_displayName;
    FutureProgress::KeepOnFinishType m_keep = FutureProgress::HideOnFinish;
};

ProcessProgressPrivate::ProcessProgressPrivate(ProcessProgress *progress, Process *process)
    : QObject(progress)
    , m_process(process)
{
}

ProcessProgressPrivate::~ProcessProgressPrivate()
{
    if (m_futureInterface.isRunning()) {
        m_futureInterface.reportCanceled();
        m_futureInterface.reportFinished();
        // TODO: should we stop the process? Or just mark the process canceled?
        // What happens to task in progress manager?
    }
}

QString ProcessProgressPrivate::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    const CommandLine commandLine = m_process->commandLine();
    QString result = commandLine.executable().baseName();
    QTC_ASSERT(!result.isEmpty(), result = Tr::tr("Unknown"));
    result[0] = result[0].toTitleCase();
    if (!commandLine.arguments().isEmpty())
        result += ' ' + commandLine.splitArguments().at(0);
    return result;
}

void ProcessProgressPrivate::parseProgress(const QString &inputText)
{
    QTC_ASSERT(m_parser, return);
    m_parser(m_futureInterface, inputText);
}

/*!
    \class Core::ProcessProgress
    \inmodule QtCreator

    \brief The ProcessProgress class is responsible for showing progress of the running process.

    It's possible to cancel the running process automatically after pressing a small 'x'
    indicator on progress panel. In this case Process::stop() method is being called.
*/

ProcessProgress::ProcessProgress(Process *process)
    : QObject(process)
    , d(new ProcessProgressPrivate(this, process))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, [this] {
        d->m_process->stop(); // TODO: See TaskProgress::setAutoStopOnCancel
    });
    connect(d->m_process, &Process::starting, this, [this] {
        d->m_futureInterface = QFutureInterface<void>();
        d->m_futureInterface.setProgressRange(0, 1);
        d->m_watcher.setFuture(d->m_futureInterface.future());
        d->m_futureInterface.reportStarted();

        const QString name = d->displayName();
        const auto id = Id::fromString(name + ".action");
        if (d->m_parser) {
            d->m_futureProgress = ProgressManager::addTask(d->m_futureInterface.future(), name, id);
        } else {
            d->m_futureProgress = ProgressManager::addTimedTask(d->m_futureInterface, name, id,
                                                   qMax(2, d->m_process->timeoutS() / 5));
        }
        d->m_futureProgress->setKeepOnFinish(d->m_keep);
    });
    connect(d->m_process, &Process::done, this, [this] {
        if (d->m_process->result() != ProcessResult::FinishedWithSuccess)
            d->m_futureInterface.reportCanceled();
        d->m_futureInterface.reportFinished();
    });
}

ProcessProgress::~ProcessProgress() = default;

void ProcessProgress::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

void ProcessProgress::setKeepOnFinish(FutureProgress::KeepOnFinishType keepType)
{
    d->m_keep = keepType;
    if (d->m_futureProgress)
        d->m_futureProgress->setKeepOnFinish(d->m_keep);
}

void ProcessProgress::setProgressParser(const ProgressParser &parser)
{
    if (d->m_parser) {
        disconnect(d->m_process, &Process::textOnStandardOutput,
                   d.get(), &ProcessProgressPrivate::parseProgress);
        disconnect(d->m_process, &Process::textOnStandardError,
                   d.get(), &ProcessProgressPrivate::parseProgress);
    }
    d->m_parser = parser;
    if (!d->m_parser)
        return;

    QTC_ASSERT(d->m_process->textChannelMode(Channel::Output) != TextChannelMode::Off,
               qWarning() << "Setting progress parser on a process without changing process' "
               "text channel mode is no-op.");

    connect(d->m_process, &Process::textOnStandardOutput,
            d.get(), &ProcessProgressPrivate::parseProgress);
    connect(d->m_process, &Process::textOnStandardError,
            d.get(), &ProcessProgressPrivate::parseProgress);
}

} // namespace Core
