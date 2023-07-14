// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsrequest.h"

#include "qbsprojectmanagertr.h"
#include "qbssession.h"

#include <projectexplorer/task.h>

#include <utils/commandline.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace QbsProjectManager::Internal {

QbsRequest::~QbsRequest()
{
    if (m_isRunning)
        m_session->cancelCurrentJob();
}

void QbsRequest::setSession(QbsSession *session)
{
    QTC_ASSERT(!m_isRunning, return);
    m_session = session;
}

void QbsRequest::start()
{
    QTC_ASSERT(!m_isRunning, return);
    QTC_ASSERT(m_session, emit done(false); return);
    QTC_ASSERT(m_requestData, emit done(false); return);

    const auto handleDone = [this](const ErrorInfo &error) {
        m_isRunning = false;
        m_session->disconnect(this);
        for (const ErrorInfoItem &item : error.items) {
            emit outputAdded(item.description, BuildStep::OutputFormat::Stdout);
            emit taskAdded(CompileTask(Task::Error, item.description, item.filePath, item.line));
        }
        emit done(error.items.isEmpty());
    };
    connect(m_session, &QbsSession::projectBuilt, this, handleDone);
    connect(m_session, &QbsSession::projectCleaned, this, handleDone);
    connect(m_session, &QbsSession::projectInstalled, this, handleDone);
    connect(m_session, &QbsSession::errorOccurred, this, [handleDone](QbsSession::Error error) {
        handleDone(ErrorInfo(QbsSession::errorString(error)));
    });
    connect(m_session, &QbsSession::taskStarted, this, [this](const QString &desciption, int max) {
        m_description = desciption;
        m_maxProgress = max;
    });
    connect(m_session, &QbsSession::maxProgressChanged, this, [this](int max) {
        m_maxProgress = max;
    });
    connect(m_session, &QbsSession::taskProgress, this, [this](int progress) {
        if (m_maxProgress > 0)
            emit progressChanged(progress * 100 / m_maxProgress, m_description);
    });
    connect(m_session, &QbsSession::commandDescription, this, [this](const QString &message) {
        emit outputAdded(message, BuildStep::OutputFormat::Stdout);
    });
    connect(m_session, &QbsSession::processResult, this, [this](const FilePath &executable,
                                                                const QStringList &arguments,
                                                                const FilePath &workingDir,
                                                                const QStringList &stdOut,
                                                                const QStringList &stdErr,
                                                                bool success) {
        Q_UNUSED(workingDir);
        const bool hasOutput = !stdOut.isEmpty() || !stdErr.isEmpty();
        if (success && !hasOutput)
            return;
        emit outputAdded(executable.toUserOutput() + ' '  + ProcessArgs::joinArgs(arguments),
                         BuildStep::OutputFormat::Stdout);
        for (const QString &line : stdErr)
            emit outputAdded(line, BuildStep::OutputFormat::Stderr);
        for (const QString &line : stdOut)
            emit outputAdded(line, BuildStep::OutputFormat::Stdout);
    });
    m_isRunning = true;
    m_session->sendRequest(*m_requestData);
}

} // namespace QbsProjectManager::Internal
