/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
    int approval;
};

class GerritPatchSet {
public:
    GerritPatchSet() : patchSetNumber(1) {}
    QString approvalsToolTip() const;
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
    QString toolTip() const;
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
    void refresh();

signals:
    void refreshStateChanged(bool isRefreshing); // For disabling the "Refresh" button.

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
