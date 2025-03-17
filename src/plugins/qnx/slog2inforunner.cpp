// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "slog2inforunner.h"

#include "qnxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Qnx::Internal {

struct SlogData
{
    RunControl *m_runControl = nullptr;
    QString m_applicationId;
    QDateTime m_launchDateTime = {};
    bool m_currentLogs = false;
    QString m_remainingData = {};

    void processLogLine(const QString &line);
    void processRemainingLogData();
    void processLogInput(const QString &input);
};

Group slog2InfoRecipe(RunControl *runControl)
{
    QString applicationId = runControl->aspectData<ExecutableAspect>()->executable.fileName();
    // See QTCREATORBUG-10712 for details.
    // We need to limit length of ApplicationId to 63 otherwise it would not match one in slog2info.
    applicationId.truncate(63);

    const Storage<SlogData> storage(SlogData{runControl, applicationId});

    const auto onTestSetup = [runControl](Process &process) {
        process.setCommand(CommandLine{runControl->device()->filePath("slog2info")});
    };
    const auto onTestDone = [runControl] {
        runControl->postMessage(Tr::tr("Warning: \"slog2info\" is not found on the device, "
                                       "debug output not available."), ErrorMessageFormat);
    };

    const auto onLaunchTimeSetup = [runControl](Process &process) {
        process.setCommand({runControl->device()->filePath("date"), "+\"%d %H:%M:%S\"", CommandLine::Raw});
    };
    const auto onLaunchTimeDone = [applicationId, storage](const Process &process) {
        QTC_CHECK(!applicationId.isEmpty());
        storage->m_launchDateTime = QDateTime::fromString(process.cleanedStdOut().trimmed(),
                                                          "dd HH:mm:ss");
    };

    const auto onLogSetup = [storage, runControl](Process &process) {
        process.setCommand({runControl->device()->filePath("slog2info"), {"-w"}});
        SlogData *slogData = storage.activeStorage();
        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [slogData, processPtr = &process] {
            slogData->processLogInput(QString::fromLatin1(processPtr->readAllRawStandardOutput()));
        });
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [runControl, processPtr = &process] {
            runControl->postMessage(QString::fromLatin1(processPtr->readAllRawStandardError()), StdErrFormat);
        });
    };
    const auto onLogError = [runControl](const Process &process) {
        runControl->postMessage(Tr::tr("Cannot show slog2info output. Error: %1")
                                    .arg(process.errorString()), StdErrFormat);
    };

    const auto onCanceled = [storage](DoneWith result) {
        if (result == DoneWith::Cancel)
            storage->processRemainingLogData();
    };

    return Group {
        storage,
        onGroupSetup([] { emit runStorage()->started(); }),
        ProcessTask(onTestSetup, onTestDone, CallDoneIf::Error),
        ProcessTask(onLaunchTimeSetup, onLaunchTimeDone, CallDoneIf::Success),
        ProcessTask(onLogSetup, onLogError, CallDoneIf::Error),
        onGroupDone(onCanceled, CallDoneIf::Error)
    }.withCancel(canceler());
}

void SlogData::processRemainingLogData()
{
    if (!m_remainingData.isEmpty())
        processLogLine(m_remainingData);
    m_remainingData.clear();
}

void SlogData::processLogInput(const QString &input)
{
    QStringList lines = input.split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return;
    lines.first().prepend(m_remainingData);
    m_remainingData = lines.takeLast();
    for (const QString &line : std::as_const(lines))
        processLogLine(line);
}

void SlogData::processLogLine(const QString &line)
{
    // The "(\\s+\\S+)?" represents a named buffer. If message has noname (aka empty) buffer
    // then the message might get cut for the first number in the message.
    // The "\\s+(\\b.*)?$" represents a space followed by a message. We are unable to determinate
    // how many spaces represent separators and how many are a part of the messages, so resulting
    // messages has all whitespaces at the beginning of the message trimmed.
    static const QRegularExpression regexp(
        "^[a-zA-Z]+\\s+([0-9]+ [0-9]+:[0-9]+:[0-9]+.[0-9]+)\\s+(\\S+)(\\s+(\\S+))?\\s+([0-9]+)\\s+(.*)?$");

    const QRegularExpressionMatch match = regexp.match(line);
    if (!match.hasMatch())
        return;

    // Note: This is useless if/once slog2info -b displays only logs from recent launches
    if (!m_launchDateTime.isNull()) {
        // Check if logs are from the recent launch
        if (!m_currentLogs) {
            const QDateTime dateTime = QDateTime::fromString(match.captured(1),
                                                             QLatin1String("dd HH:mm:ss.zzz"));
            m_currentLogs = dateTime >= m_launchDateTime;
            if (!m_currentLogs)
                return;
        }
    }

    const QString applicationId = match.captured(2);
    if (!applicationId.startsWith(m_applicationId))
        return;

    const QString bufferName = match.captured(4);
    int bufferId = match.captured(5).toInt();
    // filtering out standard BB10 messages
    if (bufferName == QLatin1String("default") && bufferId == 8900)
        return;

    m_runControl->postMessage(match.captured(6).trimmed() + '\n', StdOutFormat);
}

} // Qnx::Internal
