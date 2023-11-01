// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertraceview.h"

#include "ctfvisualizertool.h"

#include <tracing/timelineformattime.h>
#include <tracing/timelineoverviewrenderer.h>
#include <tracing/timelinerenderer.h>
#include <tracing/timelinetheme.h>

#include <QQmlContext>
#include <QQmlEngine>

namespace CtfVisualizer::Internal {

CtfVisualizerTraceView::CtfVisualizerTraceView(QWidget *parent, CtfVisualizerTool *tool)
    : QQuickWidget(parent)
{
    setObjectName(QLatin1String("CtfVisualizerTraceView"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);

    engine()->addImportPath(":/qt/qml/");
    Timeline::TimelineTheme::setupTheme(engine());

    rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                      tool->modelAggregator());
    rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                      tool->zoomControl());
    setSource(QUrl(QLatin1String("qrc:/qt/qml/QtCreator/Tracing/MainView.qml")));

    // Avoid ugly warnings when reading from null properties in QML.
    connect(tool->modelAggregator(), &QObject::destroyed, this, [this] { setSource({}); });
    connect(tool->zoomControl(), &QObject::destroyed, this, [this] { setSource({}); });
}

CtfVisualizerTraceView::~CtfVisualizerTraceView() = default;

void CtfVisualizerTraceView::selectByTypeId(int typeId)
{
    QMetaObject::invokeMethod(rootObject(), "selectByTypeId",
                              Q_ARG(QVariant,QVariant::fromValue<int>(typeId)));
}

} // namespace CtfVisualizer::Internal

