/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "flamegraphview.h"
#include "flamegraph.h"

#include <qmlprofiler/qmlprofilerconstants.h>
#include <qmlprofiler/qmlprofilertool.h>

#include <QQmlContext>
#include <QVBoxLayout>
#include <QMenu>

namespace QmlProfilerExtension {
namespace Internal {

FlameGraphView::FlameGraphView(QWidget *parent, QmlProfiler::QmlProfilerModelManager *manager) :
    QmlProfilerEventsView(parent), m_content(new QQuickWidget(this)),
    m_model(new FlameGraphModel(manager, this)), m_isRestrictedToRange(false)
{
    setWindowTitle(QStringLiteral("Flamegraph"));
    setObjectName(QStringLiteral("QmlProfilerFlamegraph"));

    qmlRegisterType<FlameGraph>("FlameGraph", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<FlameGraphModel>("FlameGraphModel", 1, 0, "FlameGraphModel",
                                                QLatin1String("use the context property"));

    m_content->rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), m_model);
    m_content->setSource(QUrl(QStringLiteral("qrc:/FlameGraphView.qml")));
    m_content->setClearColor(QColor(0xdc, 0xdc, 0xdc));

    m_content->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_content);
    setLayout(layout);

    connect(m_content->rootObject(), SIGNAL(typeSelected(int)),
            this, SIGNAL(typeSelected(int)));
    connect(m_content->rootObject(), SIGNAL(gotoSourceLocation(QString,int,int)),
            this, SIGNAL(gotoSourceLocation(QString,int,int)));
}

void FlameGraphView::clear()
{
}

void FlameGraphView::restrictToRange(qint64 rangeStart, qint64 rangeEnd)
{
    m_isRestrictedToRange = (rangeStart != -1 || rangeEnd != -1);
    m_model->loadData(rangeStart, rangeEnd);
}

bool FlameGraphView::isRestrictedToRange() const
{
    return m_isRestrictedToRange;
}

void FlameGraphView::selectByTypeId(int typeIndex)
{
    m_content->rootObject()->setProperty("selectedTypeId", typeIndex);
}

void FlameGraphView::onVisibleFeaturesChanged(quint64 features)
{
    int rangeTypeMask;
    for (int rangeType = 0; rangeType < QmlDebug::MaximumRangeType; ++rangeType) {
        if (features & (1 << QmlDebug::featureFromRangeType(QmlDebug::RangeType(rangeType))))
            rangeTypeMask |= (1 << rangeType);
    }
    m_content->rootObject()->setProperty("visibleRangeTypes", rangeTypeMask);
}

void FlameGraphView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *getGlobalStatsAction = 0;

    QPoint position = ev->globalPos();

    menu.addActions(QmlProfiler::QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();
    getGlobalStatsAction = menu.addAction(tr("Show Full Range"));
    if (!isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    if (menu.exec(position) == getGlobalStatsAction)
        emit showFullRange();
}

}
}
