// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flamegraphview.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"
#include "qmlprofilertr.h"

#include <tracing/flamegraph.h>
#include <tracing/timelinetheme.h>
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
    setWindowTitle(Tr::tr("Flame Graph"));

    m_content->engine()->addImportPath(":/qt/qml/");
    Timeline::TimelineTheme::setupTheme(m_content->engine());

    m_content->rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), m_model);
    m_content->setSource(QUrl(QStringLiteral(
                                  "qrc:/qt/qml/QtCreator/QmlProfiler/QmlProfilerFlameGraphView.qml")));
    m_content->setClearColor(Utils::creatorTheme()->color(Utils::Theme::Timeline_BackgroundColor1));

    m_content->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_content);
    setLayout(layout);

    connect(m_content->rootObject(), SIGNAL(typeSelected(int)), this, SIGNAL(typeSelected(int)));
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

    QPoint position = ev->globalPos();

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();
    QAction *getGlobalStatsAction = menu.addAction(Tr::tr("Show Full Range"));
    getGlobalStatsAction->setEnabled(m_model->modelManager()->isRestrictedToRange());
    QAction *resetAction = menu.addAction(Tr::tr("Reset Flame Graph"));
    resetAction->setEnabled(m_content->rootObject()->property("zoomed").toBool());

    const QAction *selected = menu.exec(position);
    if (selected == getGlobalStatsAction)
        emit showFullRange();
    else if (selected == resetAction)
        QMetaObject::invokeMethod(m_content->rootObject(), "resetRoot");
}

} // namespace Internal
} // namespace QmlProfiler
