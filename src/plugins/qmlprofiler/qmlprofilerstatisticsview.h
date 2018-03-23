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

#include <QPointer>
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

enum MainField {
    MainLocation,
    MainType,
    MainTimeInPercent,
    MainTotalTime,
    MainSelfTimeInPercent,
    MainSelfTime,
    MainCallCount,
    MainTimePerCall,
    MainMedianTime,
    MainMaxTime,
    MainMinTime,
    MainDetails,
    MaxMainField
};

enum RelativeField {
    RelativeLocation,
    RelativeType,
    RelativeTotalTime,
    RelativeCallCount,
    RelativeDetails,
    MaxRelativeField
};

class QmlProfilerStatisticsView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                       QWidget *parent = nullptr);
    ~QmlProfilerStatisticsView() override = default;
    void clear() override;

    QString summary(const QVector<int> &typeIds) const;
    QStringList details(int typeId) const;

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

    std::unique_ptr<QmlProfilerStatisticsMainView> m_mainView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_calleesView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_callersView;
};

class QmlProfilerStatisticsMainView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsMainView(QmlProfilerStatisticsModel *model);
    ~QmlProfilerStatisticsMainView();

    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    int selectedTypeId() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectType(int typeIndex);
    void buildModel();
    void updateNotes(int typeIndex);

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QString summary(const QVector<int> &typeIds) const;
    QStringList details(int typeId) const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);

private:
    void selectItem(const QStandardItem *item);
    void setHeaderLabels();
    void parseModel();
    QStandardItem *itemFromIndex(const QModelIndex &index) const;
    QString textForItem(QStandardItem *item) const;

    std::unique_ptr<QmlProfilerStatisticsModel> m_model;
    std::unique_ptr<QStandardItemModel> m_standardItemModel;
    bool m_showExtendedStatistics = false;
};

class QmlProfilerStatisticsRelativesView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsRelativesView(QmlProfilerStatisticsRelativesModel *model);
    ~QmlProfilerStatisticsRelativesView();

    void displayType(int typeIndex);
    void jumpToItem(const QModelIndex &);
    void clear();

signals:
    void typeClicked(int typeIndex);
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);

private:
    void rebuildTree(
            const QVector<QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData> &data);
    void updateHeader();
    QStandardItemModel *treeModel();

    std::unique_ptr<QmlProfilerStatisticsRelativesModel> m_model;
};

} // namespace Internal
} // namespace QmlProfiler
