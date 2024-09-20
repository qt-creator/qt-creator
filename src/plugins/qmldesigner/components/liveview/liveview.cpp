// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "liveview.h"
#include "liveviewwidget.h"

namespace QmlDesigner {

LiveView::LiveView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_liveViewWidget(new LiveViewWidget)
{
    QSharedMemory sharedMemory("LiveViewSharedMemory");
    if (!sharedMemory.create(32))
        qDebug() << "Warning: Unable to create shared memory segment.";
}

LiveView::~LiveView()
{
    delete m_liveViewWidget;
}

void LiveView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
}

WidgetInfo LiveView::widgetInfo()
{
    return createWidgetInfo(m_liveViewWidget.data(),
                            QStringLiteral("LiveView"),
                            WidgetInfo::LeftPane,
                            tr("Live"),
                            tr("Live View"));
}

bool LiveView::hasWidget() const
{
    return true;
}

} // namespace QmlDesigner
