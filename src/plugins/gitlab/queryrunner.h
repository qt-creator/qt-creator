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

#include <utils/id.h>
#include <utils/qtcprocess.h>

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
    Utils::QtcProcess m_process;
};

} // namespace GitLab
