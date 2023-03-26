// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorwidget.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

PropertyEditorWidget::PropertyEditorWidget(QWidget *parent) : QStackedWidget(parent)
{
    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_PROPERTYEDITOR_TIME);
}

void PropertyEditorWidget::resizeEvent(QResizeEvent * event)
{
    QStackedWidget::resizeEvent(event);
    emit resized();
}

}
