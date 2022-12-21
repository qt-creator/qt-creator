// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gerritparameters.h"
#include "gerritserver.h"

#include <QStandardItemModel>
#include <QSharedPointer>
#include <QDateTime>

namespace Gerrit {
namespace Internal {
class QueryContext;

class GerritApproval {
public:
    QString type; // Review type
    QString description; // Type description, possibly empty
    GerritUser reviewer;
    int approval = -1;
};

class GerritPatchSet {
public:
    QString approvalsToHtml() const;
    QString approvalsColumn() const;
    bool hasApproval(const GerritUser &user) const;
    int approvalLevel() const;

    QString url;
    QString ref;
    int patchSetNumber = 1;
    QList<GerritApproval> approvals;
};

class GerritChange
{
public:
    bool isValid() const { return number && !url.isEmpty() && !project.isEmpty(); }
    QString filterString() const;
    QStringList gitFetchArguments(const GerritServer &server) const;
    QString fullTitle() const; // title with potential "Draft" specifier

    QString url;
    int number = 0;
    int dependsOnNumber = 0;
    int neededByNumber = 0;
    QString title;
    GerritUser owner;
    QString project;
    QString branch;
    QString status;
    QDateTime lastUpdated;
    GerritPatchSet currentPatchSet;
    int depth = -1;
};

using GerritChangePtr = QSharedPointer<GerritChange>;

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
    GerritModel(const QSharedPointer<GerritParameters> &, QObject *parent = nullptr);
    ~GerritModel() override;

    QVariant data(const QModelIndex &index, int role) const override;

    GerritChangePtr change(const QModelIndex &index) const;
    QString toHtml(const QModelIndex &index) const;

    QStandardItem *itemForNumber(int number) const;
    QSharedPointer<GerritServer> server() const { return m_server; }

    enum QueryState { Idle, Running, Ok, Error };
    QueryState state() const { return m_state; }

    void refresh(const QSharedPointer<GerritServer> &server, const QString &query);

signals:
    void refreshStateChanged(bool isRefreshing); // For disabling the "Refresh" button.
    void stateChanged();
    void errorText(const QString &text);

private:
    void resultRetrieved(const QByteArray &);
    void queryFinished();
    void clearData();

    void setState(QueryState s);

    QString dependencyHtml(const QString &header, const int changeNumber,
                           const QString &serverPrefix) const;
    QList<QStandardItem *> changeToRow(const GerritChangePtr &c) const;

    const QSharedPointer<GerritParameters> m_parameters;
    QSharedPointer<GerritServer> m_server;
    QueryContext *m_query = nullptr;
    QueryState m_state = Idle;
};

} // namespace Internal
} // namespace Gerrit

Q_DECLARE_METATYPE(Gerrit::Internal::GerritChangePtr)
