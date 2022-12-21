// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tooltip.h"
#include <qquickwindow.h>
#include <qquickitem.h>
#include <QtQuick/QQuickRenderControl>
#include <QtWidgets/qtwidgetsglobal.h>
#include <qtooltip.h>

Tooltip::Tooltip(QObject *parent)
    : QObject(parent)
{}

void Tooltip::showText(QQuickItem *item, const QPointF &pos, const QString &str)
{
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
    QToolTip::hideText();
}

void Tooltip::registerDeclarativeType()
{
    qmlRegisterType<Tooltip>("HelperWidgets", 2, 0, "Tooltip");
}
