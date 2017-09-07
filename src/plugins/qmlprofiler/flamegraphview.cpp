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

#include "flamegraphview.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"

#include <flamegraph/flamegraph.h>
#include <timeline/timelinetheme.h>
#include <utils/theme/theme.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QVBoxLayout>
#include <QMenu>

namespace QmlProfiler {
namespace Internal {

FlameGraphView::FlameGraphView(QmlProfilerModelManager *manager, QWidget *parent) :
    QmlProfilerEventsView(parent), m_content(new QQuickWidget(this)),
    m_model(new FlameGraphModel(manager, this))
{
    setObjectName("QmlProfiler.FlameGraph.Dock");
    setWindowTitle(tr("Flame Graph"));

    qmlRegisterType<FlameGraph::FlameGraph>("FlameGraph", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<FlameGraphModel>("QmlProfilerFlameGraphModel", 1, 0,
                                                "QmlProfilerFlameGraphModel",
                                                QLatin1String("use the context property"));
    qmlRegisterUncreatableType<QAbstractItemModel>("AbstractItemModel", 1, 0, "AbstractItemModel",
                                                   QLatin1String("only for Qt 5.4"));

    Timeline::TimelineTheme::setupTheme(m_content->engine());

    m_content->rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), m_model);
    m_content->setSource(QUrl(QStringLiteral("qrc:/qmlprofiler/QmlProfilerFlameGraphView.qml")));
    m_content->setClearColor(Utils::creatorTheme()->color(Utils::Theme::Timeline_BackgroundColor1));

    m_content->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_content);
    setLayout(layout);

    connect(m_model, &FlameGraphModel::typeSelected, this, &FlameGraphView::typeSelected);
    connect(m_model, &FlameGraphModel::gotoSourceLocation,
            this, &FlameGraphView::gotoSourceLocation);
}

void FlameGraphView::selectByTypeId(int typeIndex)
{
    m_content->rootObject()->setProperty("selectedTypeId", typeIndex);
}

void FlameGraphView::onVisibleFeaturesChanged(quint64 features)
{
    m_model->restrictToFeatures(features);
}

void FlameGraphView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *getGlobalStatsAction = 0;

    QPoint position = ev->globalPos();

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();
    getGlobalStatsAction = menu.addAction(tr("Show Full Range"));
    if (!m_model->modelManager()->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    if (menu.exec(position) == getGlobalStatsAction)
        emit showFullRange();
}

} // namespace Internal
} // namespace QmlProfiler
