// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatisticsmodel.h"
#include "qmlprofilereventsview.h"

#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/itemviews.h>

#include <memory>

namespace Profiler::Internal {

class QmlProfilerStatisticsMainView;
class QmlProfilerStatisticsRelativesView;
class QmlProfilerTextMarkModel;

class QmlProfilerStatisticsView final : public QmlProfilerEventsView
{
public:
    explicit QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                       QWidget *parent = nullptr);

    QString summary(const QList<int> &typeIds) const;
    QStringList details(int typeId) const;

    void createMarks(const QString &fileName);

    void selectByTypeId(int typeIndex) final;
    void onVisibleFeaturesChanged(quint64 features) final;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) final;

private:
    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;
    bool mouseOnTable(const QPoint &position) const;

    std::unique_ptr<QmlProfilerStatisticsMainView> m_mainView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_calleesView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_callersView;
    QmlProfilerTextMarkModel *m_textMarkModel = nullptr;
};

class QmlProfilerStatisticsMainView final : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit QmlProfilerStatisticsMainView(QmlProfilerStatisticsModel *model);

    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

    void displayTypeIndex(int typeIndex);
    void jumpToItem(int typeIndex);

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QString summary(const QList<int> &typeIds) const;
    QStringList details(int typeId) const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeClicked(int typeIndex);
    void propagateTypeIndex(int typeIndex);

private:
    QString textForItem(const QModelIndex &index) const;

    std::unique_ptr<QmlProfilerStatisticsModel> m_model;
    bool m_showExtendedStatistics = false;
};

class QmlProfilerStatisticsRelativesView final : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit QmlProfilerStatisticsRelativesView(QmlProfilerStatisticsRelativesModel *model);

    void displayType(int typeIndex);
    void jumpToItem(int typeIndex);

signals:
    void typeClicked(int typeIndex);

private:
    std::unique_ptr<QmlProfilerStatisticsRelativesModel> m_model;
};

} // namespace Profiler::Internal
