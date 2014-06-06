/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "navigatortreeview.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "qproxystyle.h"

#include "metainfo.h"

#include <utils/stylehelper.h>

#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>


namespace QmlDesigner {

void drawSelectionBackground(QPainter *painter, const QStyleOption &option)
{
    painter->save();
    QLinearGradient gradient;

    QColor highlightColor = Utils::StyleHelper::notTooBrightHighlightColor();
    gradient.setColorAt(0, highlightColor.lighter(130));
    gradient.setColorAt(1, highlightColor.darker(130));
    gradient.setStart(option.rect.topLeft());
    gradient.setFinalStop(option.rect.bottomLeft());
    painter->fillRect(option.rect, gradient);
    painter->setPen(highlightColor.lighter());
    painter->drawLine(option.rect.topLeft(),option.rect.topRight());
    painter->setPen(highlightColor.darker());
    painter->drawLine(option.rect.bottomLeft(),option.rect.bottomRight());
    painter->restore();
}

// This style basically allows us to span the entire row
// including the arrow indicators which would otherwise not be
// drawn by the delegate
class TreeViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const
    {
        if (element == QStyle::PE_PanelItemViewRow) {
            if (option->state & QStyle::State_Selected) {
                drawSelectionBackground(painter, *option);
            } else {
//                // 3D shadows
//                painter->save();
//                painter->setPen(QColor(255, 255, 255, 15));
//                painter->drawLine(option->rect.topLeft(), option->rect.topRight());
//                painter->setPen(QColor(0, 0, 0, 25));
//                painter->drawLine(option->rect.bottomLeft(),option->rect.bottomRight());
//                painter->restore();
            }
        } else if (element == PE_IndicatorItemViewItemDrop) {
            painter->save();
            QRect rect = option->rect;
            rect.setLeft(0);
            rect.setWidth(widget->rect().width());
            QColor highlight = option->palette.text().color();
            highlight.setAlphaF(0.7);
            painter->setPen(QPen(highlight.lighter(), 1));
            if (option->rect.height() == 0) {
                if (option->rect.top()>0)
                    painter->drawLine(rect.topLeft(), rect.topRight());
            } else {
                highlight.setAlphaF(0.2);
                painter->setBrush(highlight);
                painter->drawRect(rect.adjusted(0, 0, -1, -1));
            }
            painter->restore();
        } else if (element == PE_FrameFocusRect) {
            // don't draw
        } else {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

NavigatorTreeView::NavigatorTreeView(QWidget *parent)
    : QTreeView(parent)
{
    TreeViewStyle *style = new TreeViewStyle;
    setStyle(style);
    style->setParent(this);
}

}
