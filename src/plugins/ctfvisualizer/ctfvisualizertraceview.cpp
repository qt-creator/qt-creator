/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
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

#include "ctfvisualizertraceview.h"

#include "ctfvisualizertool.h"

#include <tracing/timelineformattime.h>
#include <tracing/timelineoverviewrenderer.h>
#include <tracing/timelinerenderer.h>
#include <tracing/timelinetheme.h>

#include <QQmlContext>


namespace CtfVisualizer {
namespace Internal {

CtfVisualizerTraceView::CtfVisualizerTraceView(QWidget *parent, CtfVisualizerTool *tool)
    : QQuickWidget(parent)
{
    setObjectName(QLatin1String("CtfVisualizerTraceView"));

    qmlRegisterType<Timeline::TimelineRenderer>("TimelineRenderer", 1, 0, "TimelineRenderer");
    qmlRegisterType<Timeline::TimelineOverviewRenderer>("TimelineOverviewRenderer", 1, 0,
                                                        "TimelineOverviewRenderer");
    qmlRegisterType<Timeline::TimelineZoomControl>();
    qmlRegisterType<Timeline::TimelineModel>();
    qmlRegisterType<Timeline::TimelineNotesModel>();

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);

    Timeline::TimelineTheme::setupTheme(engine());
    Timeline::TimeFormatter::setupTimeFormatter();

    rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                      tool->modelAggregator());
    rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                      tool->zoomControl());
    setSource(QUrl(QLatin1String("qrc:/tracing/MainView.qml")));

    // Avoid ugly warnings when reading from null properties in QML.
    connect(tool->modelAggregator(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });
    connect(tool->zoomControl(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });
}

CtfVisualizerTraceView::~CtfVisualizerTraceView() = default;

void CtfVisualizerTraceView::selectByTypeId(int typeId)
{
    QMetaObject::invokeMethod(rootObject(), "selectByTypeId",
                              Q_ARG(QVariant,QVariant::fromValue<int>(typeId)));
}


} // namespace Internal
} // namespace CtfVisualizer

