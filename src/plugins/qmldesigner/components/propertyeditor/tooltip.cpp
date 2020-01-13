/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
