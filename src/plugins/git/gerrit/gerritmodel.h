/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITMODEL_H
#define GERRIT_INTERNAL_GERRITMODEL_H

#include <QStandardItemModel>
#include <QSharedPointer>
#include <QMetaType>
#include <QPair>
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
    GerritChange() : number(0) {}

    bool isValid() const { return number && !url.isEmpty() && !project.isEmpty(); }
    QString toHtml() const;
    QString filterString() const;
    QStringList gitFetchArguments(const QSharedPointer<GerritParameters> &p) const;

    QString url;
    int number;
    QString id;
    QString title;
    QString owner;
    QString email;
    QString project;
    QString branch;
    QString status;
    QDateTime lastUpdated;
    GerritPatchSet currentPatchSet;
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
        GerritChangeRole = Qt::UserRole + 2
    };
    GerritModel(const QSharedPointer<GerritParameters> &, QObject *parent = 0);
    ~GerritModel();

    GerritChangePtr change(int row) const;

    int indexOf(int gerritNumber) const;

public slots:
    void refresh(const QString &query);

signals:
    void refreshStateChanged(bool isRefreshing); // For disabling the "Refresh" button.
    void queryError();

private slots:
    void queryFinished(const QByteArray &);
    void queriesFinished();
    void clearData();

private:
    inline bool evaluateQuery(QString *errorMessage);

    const QSharedPointer<GerritParameters> m_parameters;
    QueryContext *m_query;
    QString m_userName;
};

} // namespace Internal
} // namespace Gerrit

Q_DECLARE_METATYPE(Gerrit::Internal::GerritChangePtr)

#endif // GERRIT_INTERNAL_GERRITMODEL_H
