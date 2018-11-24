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

#include <utils/itemviews.h>

#include <QPointer>

#include <memory>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStatisticsMainView;
class QmlProfilerStatisticsRelativesView;

class QmlProfilerStatisticsView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                       QWidget *parent = nullptr);
    ~QmlProfilerStatisticsView() override = default;

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

    std::unique_ptr<QmlProfilerStatisticsMainView> m_mainView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_calleesView;
    std::unique_ptr<QmlProfilerStatisticsRelativesView> m_callersView;
};

class QmlProfilerStatisticsMainView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsMainView(QmlProfilerStatisticsModel *model);
    ~QmlProfilerStatisticsMainView() override;

    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

    void displayTypeIndex(int typeIndex);
    void jumpToItem(int typeIndex);

    void restrictToFeatures(quint64 features);
    bool isRestrictedToRange() const;

    QString summary(const QVector<int> &typeIds) const;
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

class QmlProfilerStatisticsRelativesView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerStatisticsRelativesView(QmlProfilerStatisticsRelativesModel *model);
    ~QmlProfilerStatisticsRelativesView() override;

    void displayType(int typeIndex);
    void jumpToItem(int typeIndex);

signals:
    void typeClicked(int typeIndex);

private:

    std::unique_ptr<QmlProfilerStatisticsRelativesModel> m_model;
};

} // namespace Internal
} // namespace QmlProfiler
