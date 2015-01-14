/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

namespace {

// This style basically allows us to span the entire row
// including the arrow indicators which would otherwise not be
// drawn by the delegate
class TreeViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const
    {
        static QRect mouseOverStateSavedFrameRectangle;
        if (element == QStyle::PE_PanelItemViewRow) {
            if (option->state & QStyle::State_MouseOver)
                mouseOverStateSavedFrameRectangle = option->rect;

            if (option->state & QStyle::State_Selected) {
                NavigatorTreeView::drawSelectionBackground(painter, *option);
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
            // between elements and on elements we have a width
            if (option->rect.width() > 0) {
                m_currentTextColor = option->palette.text().color();
                QRect frameRectangle = adjustedRectangleToWidgetWidth(option->rect, widget);
                painter->save();

                if (option->rect.height() == 0) {
                    bool isNotRootItem = option->rect.top() > 10 && mouseOverStateSavedFrameRectangle.top() > 10;
                    if (isNotRootItem) {
                        drawIndicatorLine(frameRectangle.topLeft(), frameRectangle.topRight(), painter);
                        //  there is only a line in the styleoption object at this moment
                        //  so we need to use the last saved rect from the mouse over state
                        frameRectangle = adjustedRectangleToWidgetWidth(mouseOverStateSavedFrameRectangle, widget);
                        drawBackgroundFrame(frameRectangle, painter);
                    }
                } else {
                    drawHighlightFrame(frameRectangle, painter);
                }
                painter->restore();
            }
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

private: // functions
    QColor highlightBrushColor() const
    {
        QColor highlightBrushColor = m_currentTextColor;
        highlightBrushColor.setAlphaF(0.7);
        return highlightBrushColor;
    }
    QColor highlightLineColor() const
    {
        return highlightBrushColor().lighter();
    }
    QColor backgroundBrushColor() const
    {
        QColor backgroundBrushColor = highlightBrushColor();
        backgroundBrushColor.setAlphaF(0.2);
        return backgroundBrushColor;
    }
    QColor backgroundLineColor() const
    {
        return backgroundBrushColor().lighter();
    }

    void drawHighlightFrame(const QRect &frameRectangle, QPainter *painter) const
    {
        painter->setPen(QPen(highlightLineColor(), 2));
        painter->setBrush(highlightBrushColor());
        painter->drawRect(frameRectangle);
    }
    void drawBackgroundFrame(const QRect &frameRectangle, QPainter *painter) const
    {
        painter->setPen(QPen(backgroundLineColor(), 2));
        painter->setBrush(backgroundBrushColor());
        painter->drawRect(frameRectangle);
    }
    void drawIndicatorLine(const QPoint &leftPoint, const QPoint &rightPoint, QPainter *painter) const
    {
        painter->setPen(QPen(highlightLineColor(), 3));
        painter->drawLine(leftPoint, rightPoint);
    }

    QRect adjustedRectangleToWidgetWidth(const QRect &originalRectangle, const QWidget *widget) const
    {
        QRect adjustesRectangle = originalRectangle;
        adjustesRectangle.setLeft(0);
        adjustesRectangle.setWidth(widget->rect().width());
        return adjustesRectangle.adjusted(0, 0, -1, -1);
    }
private: // variables
    mutable QColor m_currentTextColor;
};

}

NavigatorTreeView::NavigatorTreeView(QWidget *parent)
    : QTreeView(parent)
{
    TreeViewStyle *style = new TreeViewStyle;
    setStyle(style);
    style->setParent(this);
}

void NavigatorTreeView::drawSelectionBackground(QPainter *painter, const QStyleOption &option)
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


}
