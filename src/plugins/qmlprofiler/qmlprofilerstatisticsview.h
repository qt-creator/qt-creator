/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatisticsmodel.h"
#include "qmlprofilereventsview.h"
#include "qmlprofilereventtypes.h"

#include <debugger/analyzer/analyzermanager.h>
#include <utils/itemviews.h>

#include <QStandardItemModel>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStatisticsMainView;
class QmlProfilerStatisticsRelativesView;

enum ItemRole {
    SortRole = Qt::UserRole + 1, // Sort by data, not by displayed string
    TypeIdRole,
    FilenameRole,
    LineRole,
    ColumnRole
};

enum Fields {
    Name,
    Callee,
    CalleeDescription,
    Caller,
    CallerDescription,
    CallCount,
    Details,
    Location,
    MaxTime,
    TimePerCall,
    SelfTime,
    SelfTimeInPercent,
    MinTime,
    TimeInPercent,
    TotalTime,
    Type,
    MedianTime,
    MaxFields
};

class QmlProfilerStatisticsView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                       QWidget *parent = nullptr);
    ~QmlProfilerStatisticsView();
    void clear() override;

    QString summary(const QVector<int> &typeIds) const;
    QStringList details(int typeId) const;

public slots:
    void selectByTypeId(int typeIndex) override;
    void onVisibleFeaturesChanged(quint64 features) override;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;
    bool mouseOnTable(const QPoint &position) const;
    void setShowExtendedStatistics(bool show);
    bool showExtendedStatistics() const;

    class QmlProfilerStatisticsViewPrivate;
    QmlProfilerStatisticsViewPrivate *d;
};

class QmlProfilerStatisticsMainView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsMainView(QWidget *parent, QmlProfilerStatisticsModel *model);
    ~QmlProfilerStatisticsMainView();

    void setFieldViewable(Fields field, bool show);

    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    static QString nameForType(RangeType typeNumber);

    int selectedTypeId() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectType(int typeIndex);
    void buildModel();
    void updateNotes(int typeIndex);

private:
    void selectItem(const QStandardItem *item);
    void setHeaderLabels();
    void parseModel();
    QStandardItem *itemFromIndex(const QModelIndex &index) const;

private:
    class QmlProfilerStatisticsMainViewPrivate;
    QmlProfilerStatisticsMainViewPrivate *d;

};

class QmlProfilerStatisticsRelativesView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsRelativesView(QmlProfilerStatisticsRelativesModel *model,
                                                QWidget *parent);
    ~QmlProfilerStatisticsRelativesView();

signals:
    void typeClicked(int typeIndex);
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);

public slots:
    void displayType(int typeIndex);
    void jumpToItem(const QModelIndex &);
    void clear();

private:
    void rebuildTree(const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesMap &map);
    void updateHeader();
    QStandardItemModel *treeModel();

    class QmlProfilerStatisticsRelativesViewPrivate;
    QmlProfilerStatisticsRelativesViewPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler
