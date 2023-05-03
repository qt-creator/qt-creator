// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/process.h>

#include <QObject>

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

signals:
    void finished();
    void resultRetrieved(const QByteArray &json);

private:
    Utils::Process m_process;
};

} // namespace GitLab
