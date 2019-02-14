/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfprofilertool.h"
#include "perfprofilertraceview.h"
#include "perftimelinemodel.h"

#include <tracing/timelineformattime.h>
#include <tracing/timelineoverviewrenderer.h>
#include <tracing/timelinerenderer.h>
#include <tracing/timelinetheme.h>

#include <QQmlContext>

namespace PerfProfiler {
namespace Internal {

PerfProfilerTraceView::PerfProfilerTraceView(QWidget *parent, PerfProfilerTool *tool)
    : QQuickWidget(parent)
{
    setObjectName(QLatin1String("PerfProfilerTraceView"));

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
                                      tool->modelManager());
    rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                      tool->zoomControl());
    setSource(QUrl(QLatin1String("qrc:/tracing/MainView.qml")));

    // Avoid ugly warnings when reading from null properties in QML.
    connect(tool->modelManager(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });
    connect(tool->zoomControl(), &QObject::destroyed, this, [this]{ setSource(QUrl()); });

    connect(tool->modelManager(), &Timeline::TimelineModelAggregator::updateCursorPosition,
            this, &PerfProfilerTraceView::updateCursorPosition);
}

bool PerfProfilerTraceView::isUsable() const
{
    return quickWindow()->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL;
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

