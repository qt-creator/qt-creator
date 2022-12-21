// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertool.h"
#include "perfprofilertraceview.h"
#include "perftimelinemodel.h"

#include <tracing/timelineformattime.h>
#include <tracing/timelineoverviewrenderer.h>
#include <tracing/timelinerenderer.h>
#include <tracing/timelinetheme.h>

#include <QQmlContext>
#include <QQmlEngine>

namespace PerfProfiler {
namespace Internal {

PerfProfilerTraceView::PerfProfilerTraceView(QWidget *parent, PerfProfilerTool *tool)
    : QQuickWidget(parent)
{
    setObjectName(QLatin1String("PerfProfilerTraceView"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);

    engine()->addImportPath(":/qt/qml/");
    Timeline::TimelineTheme::setupTheme(engine());

    rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                      tool->modelManager());
    rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                      tool->zoomControl());
    setSource(QUrl(QLatin1String("qrc:/qt/qml/QtCreator/Tracing/MainView.qml")));

    // Avoid ugly warnings when reading from null properties in QML.
    connect(tool->modelManager(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });
    connect(tool->zoomControl(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });

    connect(tool->modelManager(), &Timeline::TimelineModelAggregator::updateCursorPosition,
            this, &PerfProfilerTraceView::updateCursorPosition);
}

bool PerfProfilerTraceView::isUsable() const
{
    return QSGRendererInterface::isApiRhiBased(quickWindow()->rendererInterface()->graphicsApi());
}

void PerfProfilerTraceView::selectByTypeId(int typeId)
{
    QMetaObject::invokeMethod(rootObject(), "selectByTypeId",
                              Q_ARG(QVariant,QVariant::fromValue<int>(typeId)));
}

void PerfProfilerTraceView::clear()
{
    QMetaObject::invokeMethod(rootObject(), "clear");
}

void PerfProfilerTraceView::updateCursorPosition()
{
    QQuickItem *root = rootObject();
    QString file = root->property("fileName").toString();
    if (!file.isEmpty()) {
        emit gotoSourceLocation(file, root->property("lineNumber").toInt(),
                                root->property("columnNumber").toInt());
    }

    const PerfTimelineModelManager *manager =
            qvariant_cast<PerfTimelineModelManager *>(
                rootContext()->contextProperty(QLatin1String("timelineModelAggregator")));
    if (!manager)
        return;
    const PerfTimelineModel *model =
            static_cast<const PerfTimelineModel *>(
                manager->model(root->property("selectedModel").toInt()));
    if (!model)
        return;

    emit typeSelected(model->typeId(root->property("selectedItem").toInt()));
}

} // namespace Internal
} // namespace PerfProfiler

