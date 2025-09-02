// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

#include <solutions/tasking/tasktree.h>

#include <QJsonObject>

#include <utility>

namespace QbsProjectManager::Internal {

class QbsBuildSystem;
class QbsRequestObject;
class QbsSession;

using ParseData = std::pair<QPointer<QbsBuildSystem>, QVariantMap>;

class QbsRequest final : public QObject
{
    Q_OBJECT

public:
    ~QbsRequest() override;

    void setSession(QbsSession *session) { m_session = session; }
    void setRequestData(const QJsonObject &requestData) { m_requestData = requestData; }
    void setParseData(const ParseData &parseData) { m_parseData = parseData; }
    void start();

signals:
    void done(Tasking::DoneResult result);
    void progressChanged(int progress, const QString &info); // progress in %
    void outputAdded(const QString &output, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded(const ProjectExplorer::Task &task);

private:
    QbsSession *m_session = nullptr; // TODO: Should we keep a QPointer?
    std::optional<QJsonObject> m_requestData;
    ParseData m_parseData;
    QbsRequestObject *m_requestObject = nullptr;
};

using QbsRequestTask = Tasking::SimpleCustomTask<QbsRequest>;

} // namespace QbsProjectManager::Internal
