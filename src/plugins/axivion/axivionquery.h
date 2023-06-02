// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/process.h>

#include <QObject>

namespace Axivion::Internal {

class AxivionQuery
{
public:
    enum QueryType {NoQuery, DashboardInfo, ProjectInfo, IssuesForFileList, RuleInfo};
    explicit AxivionQuery(QueryType type, const QStringList &parameters = {});

    QString toString() const;

private:
    QueryType m_type = NoQuery;
    QStringList m_parameters;
};

class AxivionQueryRunner : public QObject
{
    Q_OBJECT
public:
    explicit AxivionQueryRunner(const AxivionQuery &query, QObject *parent = nullptr);
    void start();

signals:
    void finished();
    void resultRetrieved(const QByteArray &json);

private:
    Utils::Process m_process;
};

} // Axivion::Internal
