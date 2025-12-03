// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tooltip.h"

#include "propertyeditortracing.h"

#include <qquickwindow.h>
#include <qquickitem.h>
#include <QtQuick/QQuickRenderControl>
#include <QtWidgets/qtwidgetsglobal.h>
#include <qtooltip.h>

using QmlDesigner::PropertyEditorTracing::category;

Tooltip::Tooltip(QObject *parent)
    : QObject(parent)
{
    NanotraceHR::Tracer tracer{"tooltip constructor", category()};
}

void Tooltip::showText(QQuickItem *item, const QPointF &pos, const QString &str)
{
    NanotraceHR::Tracer tracer{"tooltip show text", category()};

    if (!item || !item->window())
        return;

    if (QCoreApplication::instance()->inherits("QApplication")) {
        QPoint quickWidgetOffsetInTlw;
        QWindow *renderWindow = QQuickRenderControl::renderWindowFor(item->window(), &quickWidgetOffsetInTlw);
        QWindow *window = renderWindow ? renderWindow : item->window();
        const QPoint offsetInQuickWidget = item->mapToScene(pos).toPoint();
        const QPoint mappedPos = window->mapToGlobal(offsetInQuickWidget + quickWidgetOffsetInTlw);
        QToolTip::showText(mappedPos, str);
    }
}

void Tooltip::hideText()
{
    NanotraceHR::Tracer tracer{"tooltip hide text", category()};

    QToolTip::hideText();
}

void Tooltip::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"tooltip register declarative type", category()};

    qmlRegisterType<Tooltip>("StudioControls", 1, 0, "ToolTipExt");
    qmlRegisterType<Tooltip>("HelperWidgets", 2, 0, "Tooltip");
}
