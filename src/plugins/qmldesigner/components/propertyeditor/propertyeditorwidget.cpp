// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorwidget.h"

#include "propertyeditortracing.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

using PropertyEditorTracing::category;

PropertyEditorWidget::PropertyEditorWidget(QWidget *parent) : QStackedWidget(parent)
{
    NanotraceHR::Tracer tracer{"property editor widget constructor", category()};

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_PROPERTYEDITOR_TIME);
}

void PropertyEditorWidget::resizeEvent(QResizeEvent * event)
{
    NanotraceHR::Tracer tracer{"property editor widget resize event", category()};

    QStackedWidget::resizeEvent(event);
    emit resized();
}

}
