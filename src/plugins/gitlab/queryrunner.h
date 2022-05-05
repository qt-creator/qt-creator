/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/qtcprocess.h>

#include <QObject>

namespace Utils { class Id; }

namespace GitLab {

class Query
{
public:
    enum Type {
        NoQuery,
        Project
    };

    explicit Query(Type type, const QStringList &parameters = {});
    QString toString() const;

private:
    Type m_type = NoQuery;
    QStringList m_parameter;
};

class QueryRunner : public QObject
{
    Q_OBJECT
public:
    QueryRunner(const Query &query, const Utils::Id &id, QObject *parent = nullptr);
    ~QueryRunner();

    void start();
    void terminate();

signals:
    void finished();
    void resultRetrieved(const QByteArray &json);

private:
    void errorTermination(const QString &msg);
    void processError(QProcess::ProcessError error);
    void processFinished();

    Utils::QtcProcess m_process;
    bool m_running = false;
};

} // namespace GitLab
