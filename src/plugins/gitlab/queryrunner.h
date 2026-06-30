// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/qtasktree.h>

#include <QObject>

#include <functional>

namespace GitLab {

class Query
{
public:
    enum Type {
        NoQuery,
        User,
        Project,
        Projects,
        Events
    };

    explicit Query(Type type, const QStringList &parameters = {});
    void setPageParameter(int page);
    void setAdditionalParameters(const QStringList &additional);
    bool hasPaginatedResults() const;
    Type type() const { return m_type; }
    QString toString() const;

private:
    Type m_type = NoQuery;
    QStringList m_parameter;
    QStringList m_additionalParameters;
    int m_pageParameter = -1;
};

class QueryRunner : public QObject
{
    Q_OBJECT
public:
    QueryRunner(const Query &query, const Utils::Id &id, QObject *parent = nullptr);
    void start();

    QByteArray result() const { return m_process.rawStdOut(); }

signals:
    void done(bool success);

private:
    Utils::Process m_process;
};

class GitLabQuery;
using QuerySetupHandler = std::function<void(GitLabQuery &)>;
using QueryDoneHandler = std::function<void(const GitLabQuery &)>;

// Configures and carries the result of a single gitLabQuery() task. Set the query and the
// server in the task's setup handler; read the response body in its done handler.
class GitLabQuery
{
public:
    void setQuery(const Query &query) { m_query = query; }
    void setServerId(const Utils::Id &id) { m_id = id; }
    QByteArray result() const { return m_result; }

private:
    friend QtTaskTree::Group gitLabQuery(const QuerySetupHandler &, const QueryDoneHandler &);
    Query m_query{Query::NoQuery};
    Utils::Id m_id;
    QByteArray m_result;
};

// Runs a single GitLab query as a composable subtree: a curl process and, only on an SSL
// certificate error, an asynchronous prompt followed by at most one retry that ignores the
// certificate. onSetup configures the query and server; on success onDone is invoked with the
// query whose result() holds the response body. A process failure or a rejected prompt stops
// the subtree with DoneResult::Error without invoking onDone.
QtTaskTree::Group gitLabQuery(const QuerySetupHandler &onSetup, const QueryDoneHandler &onDone);

} // namespace GitLab
