// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsrequest.h"

#include "qbsproject.h"
#include "qbssession.h"

#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <utils/commandline.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager::Internal {

class QbsRequestManager : public QObject
{
public:
    void sendRequest(QbsRequestObject *requestObject);
    void cancelRequest(QbsRequestObject *requestObject);
private:
    void continueSessionQueue(QbsSession *session);
    QHash<QObject *, QList<QbsRequestObject *>> m_queuedRequests;
};

class QbsRequestObject final : public QObject
{
    Q_OBJECT

public:
    void setSession(QbsSession *session) { m_session = session; }
    QbsSession *session() const { return m_session; }
    void setRequestData(const QJsonObject &requestData) { m_requestData = requestData; }
    void setParseData(const QPointer<QbsBuildSystem> &buildSystem) { m_parseData = buildSystem; }
    void start();
    void cancel();

signals:
    void done(bool success);
    void progressChanged(int progress, const QString &info); // progress in %
    void outputAdded(const QString &output, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded(const ProjectExplorer::Task &task);

private:
    QbsSession *m_session = nullptr;
    QJsonObject m_requestData;
    QPointer<QbsBuildSystem> m_parseData;
    QString m_description;
    int m_maxProgress = 100;
};

void QbsRequestManager::sendRequest(QbsRequestObject *requestObject)
{
    QbsSession *session = requestObject->session();
    QList<QbsRequestObject *> &queue = m_queuedRequests[session];
    if (queue.isEmpty()) {
        connect(session, &QObject::destroyed, this, [this, session] {
            qDeleteAll(m_queuedRequests.value(session));
            m_queuedRequests.remove(session);
        });
    }
    queue.append(requestObject);
    if (queue.size() == 1)
        continueSessionQueue(session);
}

void QbsRequestManager::cancelRequest(QbsRequestObject *requestObject)
{
    QbsSession *session = requestObject->session();
    QList<QbsRequestObject *> &queue = m_queuedRequests[session];
    const int index = queue.indexOf(requestObject);
    QTC_ASSERT(index >= 0, return);
    if (index > 0) {
        delete queue.takeAt(index);
        return;
    }
    requestObject->cancel();
}

void QbsRequestManager::continueSessionQueue(QbsSession *session)
{
    const QList<QbsRequestObject *> &queue = m_queuedRequests[session];
    if (queue.isEmpty()) {
        m_queuedRequests.remove(session);
        disconnect(session, &QObject::destroyed, this, nullptr);
        return;
    }
    QbsRequestObject *requestObject = queue.first();
    connect(requestObject, &QbsRequestObject::done, this, [this, requestObject] {
        disconnect(requestObject, &QbsRequestObject::done, this, nullptr);
        QbsSession *session = requestObject->session();
        requestObject->deleteLater();
        QList<QbsRequestObject *> &queue = m_queuedRequests[session];
        QTC_ASSERT(!queue.isEmpty(), return);
        QTC_CHECK(queue.first() == requestObject);
        queue.removeFirst();
        continueSessionQueue(session);
    });
    requestObject->start();
}

static QbsRequestManager &manager()
{
    static QbsRequestManager theManager;
    return theManager;
}

void QbsRequestObject::start()
{
    if (m_parseData) {
        connect(m_parseData->target(), &Target::parsingFinished, this, [this](bool success) {
            disconnect(m_parseData->target(), &Target::parsingFinished, this, nullptr);
            emit done(success);
        });
        QMetaObject::invokeMethod(m_parseData.get(), &QbsBuildSystem::startParsing,
                                  Qt::QueuedConnection);
        return;
    }

    const auto handleDone = [this](const ErrorInfo &error) {
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
    m_session->sendRequest(m_requestData);
}

void QbsRequestObject::cancel()
{
    if (m_parseData)
        m_parseData->cancelParsing();
    else
        m_session->cancelCurrentJob();
}

QbsRequest::~QbsRequest()
{
    if (!m_requestObject)
        return;
    disconnect(m_requestObject, nullptr, this, nullptr);
    manager().cancelRequest(m_requestObject);
}

void QbsRequest::start()
{
    QTC_ASSERT(!m_requestObject, return);
    QTC_ASSERT(m_parseData || (m_session && m_requestData), emit done(false); return);

    m_requestObject = new QbsRequestObject;
    m_requestObject->setSession(m_session);
    if (m_requestData)
        m_requestObject->setRequestData(*m_requestData);
    if (m_parseData) {
        m_requestObject->setSession(m_parseData->session());
        m_requestObject->setParseData(m_parseData);
    }

    connect(m_requestObject, &QbsRequestObject::done, this, [this](bool success) {
        m_requestObject->deleteLater();
        m_requestObject = nullptr;
        emit done(success);
    });
    connect(m_requestObject, &QbsRequestObject::progressChanged,
            this, &QbsRequest::progressChanged);
    connect(m_requestObject, &QbsRequestObject::outputAdded, this, &QbsRequest::outputAdded);
    connect(m_requestObject, &QbsRequestObject::taskAdded, this, &QbsRequest::taskAdded);

    manager().sendRequest(m_requestObject);
}

} // namespace QbsProjectManager::Internal

#include "qbsrequest.moc"
