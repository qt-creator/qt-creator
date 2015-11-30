/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITMODEL_H
#define GERRIT_INTERNAL_GERRITMODEL_H

#include <QStandardItemModel>
#include <QSharedPointer>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Gerrit {
namespace Internal {
class GerritParameters;
class QueryContext;

class GerritApproval {
public:
    GerritApproval() : approval(-1) {}

    QString type; // Review type
    QString description; // Type description, possibly empty
    QString reviewer;
    QString email;
    int approval;
};

class GerritPatchSet {
public:
    GerritPatchSet() : patchSetNumber(1) {}
    QString approvalsToHtml() const;
    QString approvalsColumn() const;
    bool hasApproval(const QString &userName) const;
    int approvalLevel() const;

    QString ref;
    int patchSetNumber;
    QList<GerritApproval> approvals;
};

class GerritChange
{
public:
    bool isValid() const { return number && !url.isEmpty() && !project.isEmpty(); }
    QString filterString() const;
    QStringList gitFetchArguments(const QSharedPointer<GerritParameters> &p) const;

    QString url;
    int number = 0;
    int dependsOnNumber = 0;
    int neededByNumber = 0;
    QString title;
    QString owner;
    QString email;
    QString project;
    QString branch;
    QString status;
    QDateTime lastUpdated;
    GerritPatchSet currentPatchSet;
    int depth = -1;
};

typedef QSharedPointer<GerritChange> GerritChangePtr;

class GerritModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum Columns {
        NumberColumn,
        TitleColumn,
        OwnerColumn,
        DateColumn,
        ProjectColumn,
        ApprovalsColumn,
        StatusColumn,
        ColumnCount
    };

    enum CustomModelRoles {
        FilterRole = Qt::UserRole + 1,
        GerritChangeRole = Qt::UserRole + 2,
        SortRole = Qt::UserRole + 3
    };
    GerritModel(const QSharedPointer<GerritParameters> &, QObject *parent = 0);
    ~GerritModel();

    QVariant data(const QModelIndex &index, int role) const override;

    GerritChangePtr change(const QModelIndex &index) const;
    QString toHtml(const QModelIndex &index) const;

    QStandardItem *itemForNumber(int number) const;

    enum QueryState { Idle, Running, Ok, Error };
    QueryState state() const { return m_state; }

    void refresh(const QString &query);

signals:
    void refreshStateChanged(bool isRefreshing); // For disabling the "Refresh" button.
    void stateChanged();

private:
    void queryFinished(const QByteArray &);
    void queriesFinished();
    void clearData();

    void setState(QueryState s);

    QString dependencyHtml(const QString &header, const int changeNumber,
                           const QString &serverPrefix) const;
    QList<QStandardItem *> changeToRow(const GerritChangePtr &c) const;

    const QSharedPointer<GerritParameters> m_parameters;
    QueryContext *m_query = 0;
    QueryState m_state = Idle;
    QString m_userName;
};

} // namespace Internal
} // namespace Gerrit

Q_DECLARE_METATYPE(Gerrit::Internal::GerritChangePtr)

#endif // GERRIT_INTERNAL_GERRITMODEL_H
