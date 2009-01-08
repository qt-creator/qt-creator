/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "manhattanstyle.h"

#include "stylehelper.h"
#include "styleanimator.h"

#include <QtCore/QDebug>
#include <QtCore/QLibrary>

#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QDockWidget>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPixmapCache>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QStyleFactory>
#include <QtGui/QStyleOption>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>

#include <utils/qtcassert.h>

// We define a currently unused state for indicating animations
#define State_Animating 0x00000040

// Because designer needs to disable this for widget previews
// we have a custom property that is inherited
bool styleEnabled(const QWidget *widget)
{
    const QWidget *p = widget;
    while (p) {
        if (p->property("_q_custom_style_disabled").toBool())
            return false;
            p = p->parentWidget();
    }
    return true;
}

// Consider making this a QStyle state
bool panelWidget(const QWidget *widget)
{
    const QWidget *p = widget;

    while (p) {
        if (qobject_cast<const QToolBar *>(p) && styleEnabled(p))
            return true;
        else if (qobject_cast<const QStatusBar *>(p) && styleEnabled(p))
            return true;
        else if (qobject_cast<const QMenuBar *>(p) && styleEnabled(p))
            return true;
        p = p->parentWidget();
    }
    return false;
}

class ManhattanStylePrivate
{
public:
    ManhattanStylePrivate(const QString &baseStyleName)
    {
        style = QStyleFactory::create(baseStyleName);
        QTC_ASSERT(style, /**/);
        buttonImage_pressed = QImage(":/qworkbench/images/pushbutton_pressed.png");
        buttonImage = QImage(":/qworkbench/images/pushbutton.png");

        lineeditImage = QImage(":/qworkbench/images/inputfield.png");
        lineeditImage_disabled = QImage(":/qworkbench/images/inputfield_disabled.png");
    }

    ~ManhattanStylePrivate()
    {
        delete style;
        style = 0;
    }

    void init();

public:
    QStyle *style;
    QImage buttonImage;
    QImage buttonImage_pressed;
    QImage lineeditImage;
    QImage lineeditImage_disabled;

    StyleAnimator animator;
};

ManhattanStyle::ManhattanStyle(const QString &baseStyleName)
    : QWindowsStyle(), d(new ManhattanStylePrivate(baseStyleName))
{
}

ManhattanStyle::~ManhattanStyle()
{
    delete d;
    d = 0;
}

// Draws a CSS-like border image where the defined borders are not stretched
void drawCornerImage(const QImage &img, QPainter *painter, QRect rect,
                     int left = 0, int top = 0, int right = 0,
                     int bottom = 0)
{
    QSize size = img.size();
    if (top > 0) { //top
        painter->drawImage(QRect(rect.left() + left, rect.top(), rect.width() -right - left, top), img,
                           QRect(left, 0, size.width() -right - left, top));
        if (left > 0) //top-left
            painter->drawImage(QRect(rect.left(), rect.top(), left, top), img,
                               QRect(0, 0, left, top));
        if (right > 0) //top-right
            painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top(), right, top), img,
                               QRect(size.width() - right, 0, right, top));
    }
    //left
    if (left > 0)
        painter->drawImage(QRect(rect.left(), rect.top()+top, left, rect.height() - top - bottom), img,
                           QRect(0, top, left, size.height() - bottom - top));
    //center
    painter->drawImage(QRect(rect.left() + left, rect.top()+top, rect.width() -right - left,
                             rect.height() - bottom - top), img,
                       QRect(left, top, size.width() -right -left,
                             size.height() - bottom - top));
    if (right > 0) //right
        painter->drawImage(QRect(rect.left() +rect.width() - right, rect.top()+top, right, rect.height() - top - bottom), img,
                           QRect(size.width() - right, top, right, size.height() - bottom - top));
    if (bottom > 0) { //bottom
        painter->drawImage(QRect(rect.left() +left, rect.top() + rect.height() - bottom,
                                 rect.width() - right - left, bottom), img,
                           QRect(left, size.height() - bottom,
                                 size.width() - right - left, bottom));
    if (left > 0) //bottom-left
        painter->drawImage(QRect(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img,
                           QRect(0, size.height() - bottom, left, bottom));
    if (right > 0) //bottom-right
        painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img,
                           QRect(size.width() - right, size.height() - bottom, right, bottom));
    }
}

QPixmap ManhattanStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const
{
    QPixmap result;
    result = d->style->generatedIconPixmap(iconMode, pixmap, opt);
    return result;
}

int ManhattanStyle::layoutSpacingImplementation(QSizePolicy::ControlType control1,
                                                 QSizePolicy::ControlType control2,
                                                 Qt::Orientation orientation,
                                                 const QStyleOption * option ,
                                                 const QWidget * widget ) const
{
    return d->style->layoutSpacing(control1, control2, orientation, option, widget);

}

QSize ManhattanStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                       const QSize &size, const QWidget *widget) const
{
    if (type == CT_Splitter && widget && widget->property("minisplitter").toBool())
        return QSize(1, 1);
    return d->style->sizeFromContents(type, option, size, widget);
}

QRect ManhattanStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    QRect rect;
    rect = d->style->subElementRect(element, option, widget);
    return rect;
}

QRect ManhattanStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                     SubControl subControl, const QWidget *widget) const
{
    QRect rect;
    rect = d->style->subControlRect(control, option, subControl, widget);
    return rect;
}

QStyle::SubControl ManhattanStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                         const QPoint &pos, const QWidget *widget) const
{
    SubControl result = QStyle::SC_None;
    result = d->style->hitTestComplexControl(control, option, pos, widget);
    return result;
}

int ManhattanStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int retval = 0;
    retval = d->style->pixelMetric(metric, option, widget);
    switch (metric) {
    case PM_SplitterWidth:
        if (widget && widget->property("minisplitter").toBool())
            retval = 1;
        break;
    case PM_ToolBarIconSize:
        if (panelWidget(widget))
            retval = 16;
        break;
    case PM_MenuPanelWidth:
    case PM_MenuBarHMargin:
    case PM_MenuBarVMargin:
    case PM_ToolBarFrameWidth:
        if (panelWidget(widget))
            retval = 1;
        break;
    case PM_ButtonShiftVertical:
    case PM_ButtonShiftHorizontal:
    case PM_MenuBarPanelWidth:
    case PM_ToolBarItemMargin:
    case PM_ToolBarItemSpacing:
        if (panelWidget(widget))
            retval = 0;
        break;
    case PM_DefaultFrameWidth:
        if (qobject_cast<const QLineEdit*>(widget) && panelWidget(widget))
            return 1;
        break;
    default:
        break;
    }
    return retval;
}

QPalette ManhattanStyle::standardPalette() const
{
    QPalette result;
    result = d->style->standardPalette();
    return result;
}

void ManhattanStyle::polish(QApplication *app)
{
    d->style->polish(app);
}

void ManhattanStyle::unpolish(QApplication *app)
{
    d->style->unpolish(app);
}

QPalette panelPalette(const QPalette &oldPalette)
{
    QColor color = StyleHelper::panelTextColor();
    QPalette pal = oldPalette;
    pal.setBrush(QPalette::All, QPalette::WindowText, color);
    pal.setBrush(QPalette::All, QPalette::ButtonText, color);
    pal.setBrush(QPalette::All, QPalette::Foreground, color);
    color.setAlpha(100);
    pal.setBrush(QPalette::Disabled, QPalette::WindowText, color);
    pal.setBrush(QPalette::Disabled, QPalette::ButtonText, color);
    pal.setBrush(QPalette::Disabled, QPalette::Foreground, color);
    return pal;
}

void ManhattanStyle::polish(QWidget *widget)
{
    d->style->polish(widget);

    // OxygenStyle forces a rounded widget mask on toolbars
    if (d->style->inherits("OxygenStyle")) {
        if (qobject_cast<QToolBar*>(widget))
            widget->removeEventFilter(d->style);
    }
    if (panelWidget(widget)) {
        if (qobject_cast<QToolButton*>(widget)) {
            widget->setAttribute(Qt::WA_Hover);
            widget->setMaximumHeight(StyleHelper::navigationWidgetHeight() - 2);
        }
        else if (qobject_cast<QLineEdit*>(widget)) {
            widget->setAttribute(Qt::WA_Hover);
            widget->setMaximumHeight(StyleHelper::navigationWidgetHeight() - 2);
        }
        else if (qobject_cast<QLabel*>(widget))
            widget->setPalette(panelPalette(widget->palette()));
        else if (qobject_cast<QToolBar*>(widget))
            widget->setMinimumHeight(StyleHelper::navigationWidgetHeight());
        else if (qobject_cast<QStatusBar*>(widget))
            widget->setFixedHeight(StyleHelper::navigationWidgetHeight() + 2);
        else if (qobject_cast<QComboBox*>(widget))
            widget->setMaximumHeight(StyleHelper::navigationWidgetHeight() - 2);
    }
}

void ManhattanStyle::unpolish(QWidget *widget)
{
    d->style->unpolish(widget);
    if (panelWidget(widget)) {
        if (qobject_cast<QTabBar*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
        else if (qobject_cast<QToolBar*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
    }
}

void ManhattanStyle::polish(QPalette &pal)
{
    d->style->polish(pal);
}

QIcon ManhattanStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                                 const QWidget *widget) const
{
    static const QIcon closeButton(":/qworkbench/images/closebutton.png");
    QIcon icon;
    switch (standardIcon) {
    case QStyle::SP_TitleBarCloseButton:
        icon = closeButton;
        break;
    default:
        icon = d->style->standardIcon(standardIcon, option, widget);
    }
    return icon;
}

QPixmap ManhattanStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                       const QWidget *widget) const
{
    static const QPixmap closeButton(":/qworkbench/images/closebutton.png");
    QPixmap pixmap;
    switch (standardPixmap) {
    case QStyle::SP_TitleBarCloseButton:
        pixmap = closeButton;
        break;
    default:
        pixmap = d->style->standardPixmap(standardPixmap, opt, widget);
    }
    return pixmap;
}


int ManhattanStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                              QStyleHintReturn *returnData) const
{
    int ret = 0;
    switch (hint) {
    case QStyle::SH_EtchDisabledText:
        ret = false; // We really should only enforce this for panel widgets
        break;
    default:
        ret =  d->style->styleHint(hint, option, widget, returnData);
    }

    return ret;
}

void ManhattanStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
        return d->style->drawPrimitive(element, option, painter, widget);

    bool animating = (option->state & State_Animating);
    int state = option->state;
    QRect rect = option->rect;
    QRect oldRect;
    QRect newRect;
    if (widget && (element == PE_PanelButtonTool) && !animating) {
        QWidget *w = const_cast<QWidget *> (widget);
        int oldState = w->property("_q_stylestate").toInt();
        oldRect = w->property("_q_stylerect").toRect();
        newRect = w->rect();
        w->setProperty("_q_stylestate", (int)option->state);
        w->setProperty("_q_stylerect", w->rect());

        // Determine the animated transition
        bool doTransition = ((state & State_On)         != (oldState & State_On)     ||
                             (state & State_MouseOver)  != (oldState & State_MouseOver));
        if (oldRect != newRect)
        {
            doTransition = false;
            d->animator.stopAnimation(widget);
        }

        if (doTransition) {
            QImage startImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
            QImage endImage(option->rect.size(), QImage::Format_ARGB32_Premultiplied);
            Animation *anim = d->animator.widgetAnimation(widget);
            QStyleOption opt = *option;
            opt.state = (QStyle::State)oldState;
            opt.state |= (State)State_Animating;
            startImage.fill(0);
            Transition *t = new Transition;
            t->setWidget(w);
            QPainter startPainter(&startImage);
            if (!anim) {
                drawPrimitive(element, &opt, &startPainter, widget);
            } else {
                anim->paint(&startPainter, &opt);
                d->animator.stopAnimation(widget);
            }
            QStyleOption endOpt = *option;
            endOpt.state |= (State)State_Animating;
            t->setStartImage(startImage);
            d->animator.startAnimation(t);
            endImage.fill(0);
            QPainter endPainter(&endImage);
            drawPrimitive(element, &endOpt, &endPainter, widget);
            t->setEndImage(endImage);
            t->setDuration(130);
            t->setStartTime(QTime::currentTime());
        }
    }

    switch (element) {
    case PE_PanelLineEdit:
        {
            painter->save();
            if (option->state & State_Enabled)
                drawCornerImage(d->lineeditImage, painter, option->rect, 2, 2, 2, 2);
            else
                drawCornerImage(d->lineeditImage_disabled, painter, option->rect, 2, 2, 2, 2);

            if (option->state & State_HasFocus || option->state & State_MouseOver) {
                QColor hover = StyleHelper::baseColor();
                if (state & State_HasFocus)
                    hover.setAlpha(100);
                else
                    hover.setAlpha(50);

                painter->setPen(QPen(hover, 1));
                painter->drawRect(option->rect.adjusted(1, 1, -2 ,-2));
            }
            painter->restore();
        }
        break;

    case PE_FrameStatusBarItem:
        break;

    case PE_PanelButtonTool: {
            Animation *anim = d->animator.widgetAnimation(widget);
            if (!animating && anim) {
                anim->paint(painter, option);
            } else {
                bool pressed = option->state & State_Sunken || option->state & State_On;
                QColor shadow(0, 0, 0, 30);
                painter->setPen(shadow);
                if (pressed) {
                    QColor shade(0, 0, 0, 50);
                    if (option->state & State_Sunken)
                        shade = QColor(0, 0, 0, 70);
#ifndef Q_WS_MAC
                    else if (option->state & State_MouseOver)
                        shade = QColor(255, 255, 255, 40);
#endif
                    else if (option->state & State_On)
                        shade = QColor(0, 0, 0, 50);
                    else
                        shade = QColor(0, 0, 0, 0);
                    painter->fillRect(rect.adjusted(1, 1, -1, -1), shade);
                    painter->drawLine(rect.topLeft(), rect.topRight());
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                    QColor highlight(255, 255, 255, 30);
                    painter->setPen(highlight);
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                }
 #ifndef Q_WS_MAC
               else if (option->state & State_MouseOver) {
                    QColor lighter(255, 255, 255, 100);
                    painter->fillRect(rect, lighter);
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                }
#endif
           }
        }
        break;

    case PE_PanelStatusBar:
        {
            painter->save();
            QLinearGradient grad(option->rect.topLeft(), QPoint(rect.center().x(), rect.bottom()));
            QColor startColor = StyleHelper::shadowColor().darker(164);
            QColor endColor = StyleHelper::baseColor().darker(130);
            grad.setColorAt(0, startColor);
            grad.setColorAt(1, endColor);
            painter->fillRect(option->rect, grad);
            painter->setPen(QColor(255, 255, 255, 60));
            painter->drawLine(rect.topLeft() + QPoint(0,1),
                              rect.topRight()+ QPoint(0,1));
            painter->setPen(StyleHelper::borderColor().darker(110));
            painter->drawLine(rect.topLeft(), rect.topRight());
            painter->restore();
        }
        break;

    case PE_IndicatorToolBarSeparator:
        {
            QColor separatorColor = StyleHelper::borderColor();
            separatorColor.setAlpha(100);
            painter->setPen(separatorColor);
            const int margin = 6;
            if (option->state & State_Horizontal) {
                const int offset = rect.width()/2;
                painter->drawLine(rect.bottomLeft().x() + offset,
                            rect.bottomLeft().y() - margin,
                            rect.topLeft().x() + offset,
                            rect.topLeft().y() + margin);
            } else { //Draw vertical separator
                const int offset = rect.height()/2;
                painter->setPen(QPen(option->palette.background().color().darker(110)));
                painter->drawLine(rect.topLeft().x() + margin ,
                            rect.topLeft().y() + offset,
                            rect.topRight().x() - margin,
                            rect.topRight().y() + offset);
            }
        }
        break;

    case PE_IndicatorToolBarHandle:
        {
            bool horizontal = option->state & State_Horizontal;
            painter->save();
            QPainterPath path;
            int x = option->rect.x() + horizontal ? 2 : 6;
            int y = option->rect.y() + horizontal ? 6 : 2;
            static const int RectHeight = 2;
            if (horizontal) {
                while (y < option->rect.height() - RectHeight - 6) {
                    path.moveTo(x, y);
                    path.addRect(x, y, RectHeight, RectHeight);
                    y += 6;
                }
            } else {
                while (x < option->rect.width() - RectHeight - 6) {
                    path.moveTo(x, y);
                    path.addRect(x, y, RectHeight, RectHeight);
                    x += 6;
                }
            }

            painter->setPen(Qt::NoPen);
            QColor dark = StyleHelper::borderColor();
            dark.setAlphaF(0.4);

            QColor light = StyleHelper::baseColor();
            light.setAlphaF(0.4);

            painter->fillPath(path, light);
            painter->save();
            painter->translate(1, 1);
            painter->fillPath(path, dark);
            painter->restore();
            painter->translate(3, 3);
            painter->fillPath(path, light);
            painter->translate(1, 1);
            painter->fillPath(path, dark);
            painter->restore();
        }
        break;

    default:
        d->style->drawPrimitive(element, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawControl(ControlElement element, const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
        return d->style->drawControl(element, option, painter, widget);

    switch (element) {
    case CE_MenuBarItem:
        painter->save();
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QColor highlightOutline = StyleHelper::borderColor().lighter(120);
            bool act = mbi->state & State_Selected && mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);
            StyleHelper::menuGradient(painter, option->rect, option->rect);
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect;
            QPalette pal = mbi->palette;
            pal.setBrush(QPalette::ButtonText, dis ? Qt::gray : Qt::black);
            item.palette = pal;
            QCommonStyle::drawControl(element, &item, painter, widget);
            QRect r = option->rect;

            if (act) {
                // Fill|
                QColor baseColor = StyleHelper::baseColor();
                QLinearGradient grad(option->rect.topLeft(), option->rect.bottomLeft());
                grad.setColorAt(0, baseColor.lighter(120));
                grad.setColorAt(1, baseColor.lighter(130));
                painter->fillRect(option->rect.adjusted(1, 1, -1, 0), grad);

                // Outline
                painter->setPen(QPen(highlightOutline, 0));
                painter->drawLine(QPoint(r.left(), r.top() + 1), QPoint(r.left(), r.bottom()));
                painter->drawLine(QPoint(r.right(), r.top() + 1), QPoint(r.right(), r.bottom()));
                painter->drawLine(QPoint(r.left() + 1, r.top()), QPoint(r.right() - 1, r.top()));
                highlightOutline.setAlpha(60);
                painter->setPen(QPen(highlightOutline, 0));
                painter->drawPoint(r.topLeft());
                painter->drawPoint(r.topRight());

                QPalette pal = mbi->palette;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;
                pal.setBrush(QPalette::Text, dis ? Qt::gray : QColor(0, 0, 0, 60));
                drawItemText(painter, item.rect.translated(0, 1), alignment, pal, mbi->state & State_Enabled, mbi->text, QPalette::Text);
                pal.setBrush(QPalette::Text, dis ? Qt::gray : Qt::white);
                drawItemText(painter, item.rect, alignment, pal, mbi->state & State_Enabled, mbi->text, QPalette::Text);
            }
        }
        painter->restore();
        break;

    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (panelWidget(widget)) {
                QRect editRect = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
                QPalette customPal = cb->palette;

                if (!cb->currentIcon.isNull()) {
                    QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal
                                                                 : QIcon::Disabled;
                    QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
                    QRect iconRect(editRect);
                    iconRect.setWidth(cb->iconSize.width() + 4);
                    iconRect = alignedRect(cb->direction,
                                           Qt::AlignLeft | Qt::AlignVCenter,
                                           iconRect.size(), editRect);
                    if (cb->editable)
                        painter->fillRect(iconRect, customPal.brush(QPalette::Base));
                    drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                    if (cb->direction == Qt::RightToLeft)
                        editRect.translate(-4 - cb->iconSize.width(), 0);
                    else
                        editRect.translate(cb->iconSize.width() + 4, 0);
                }

                customPal.setBrush(QPalette::All, QPalette::ButtonText, QColor(0, 0, 0, 70));

                QRect rect = editRect.adjusted(1, 0, -8, 0);
                QString text = option->fontMetrics.elidedText(cb->currentText, Qt::ElideRight, rect.width());
                drawItemText(painter, rect.translated(0, 1),
                             visualAlignment(option->direction, Qt::AlignLeft | Qt::AlignVCenter),
                             customPal, cb->state & State_Enabled, text, QPalette::ButtonText);
                customPal.setBrush(QPalette::All, QPalette::ButtonText, StyleHelper::panelTextColor());
                drawItemText(painter, rect,
                             visualAlignment(option->direction, Qt::AlignLeft | Qt::AlignVCenter),
                             customPal, cb->state & State_Enabled, text, QPalette::ButtonText);
            } else {
                d->style->drawControl(element, option, painter, widget);
            }
        }
        break;

    case CE_SizeGrip: {
            painter->save();
            QColor dark = Qt::white;
            dark.setAlphaF(0.1);
            int x, y, w, h;
            option->rect.getRect(&x, &y, &w, &h);
            int sw = qMin(h, w);
            if (h > w)
                painter->translate(0, h - w);
            else
                painter->translate(w - h, 0);
            int sx = x;
            int sy = y;
            int s = 4;
            painter->setPen(dark);
            if (option->direction == Qt::RightToLeft) {
                sx = x + sw;
                for (int i = 0; i < 4; ++i) {
                    painter->drawLine(x, sy, sx, sw);
                    sx -= s;
                    sy += s;
                }
            } else {
                for (int i = 0; i < 4; ++i) {
                    painter->drawLine(sx, sw, sw, sy);
                    sx += s;
                    sy += s;
                }
            }
            painter->restore();
        }
        break;

    case CE_MenuBarEmptyArea: {
            StyleHelper::menuGradient(painter, option->rect, option->rect);
            painter->save();
            painter->setPen(StyleHelper::borderColor());
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            painter->restore();
        }
        break;

    case CE_ToolBar:
        {
            QString key;
            key.sprintf("mh_toolbar %d %d %d", option->rect.width(), option->rect.height(), StyleHelper::baseColor().rgb());;

            QPixmap pixmap;
            QPainter *p = painter;
            QRect rect = option->rect;
            if (StyleHelper::usePixmapCache() && !QPixmapCache::find(key, pixmap)) {
                pixmap = QPixmap(option->rect.size());
                p = new QPainter(&pixmap);
                rect = QRect(0, 0, option->rect.width(), option->rect.height());
            }

            bool horizontal = option->state & State_Horizontal;
            // Map offset for global window gradient
            QPoint offset = widget->window()->mapToGlobal(option->rect.topLeft()) -
                                                          widget->mapToGlobal(option->rect.topLeft());
            QRect gradientSpan;
            if (widget) {
                gradientSpan = QRect(offset, widget->window()->size());
            }
            if (horizontal)
                StyleHelper::horizontalGradient(p, gradientSpan, rect);
            else
                StyleHelper::verticalGradient(p, gradientSpan, rect);

            painter->setPen(StyleHelper::borderColor());

            if (horizontal) {
                // Note: This is a hack to determine if we are the topmost
                // toolbar and menu bar should draw the outline
                QColor lighter(255, 255, 255, 40);
                if (widget->mapToParent(rect.topLeft()).y()) {
                    p->drawLine(rect.topLeft(), rect.topRight());
                    p->setPen(lighter);
                    p->drawLine(rect.topLeft() + QPoint(0, 1), rect.topRight() + QPoint(0, 1));
                } else {
                    p->drawLine(rect.bottomLeft(), rect.bottomRight());
                    p->setPen(lighter);
                    p->drawLine(rect.topLeft(), rect.topRight());
                }
            } else {
                p->drawLine(rect.topLeft(), rect.bottomLeft());
                p->drawLine(rect.topRight(), rect.bottomRight());
            }

            if (StyleHelper::usePixmapCache() && !QPixmapCache::find(key, pixmap)) {
                painter->drawPixmap(rect.topLeft(), pixmap);
                p->end();
                delete p;
                QPixmapCache::insert(key, pixmap);
            }
        }
        break;

    default:
        d->style->drawControl(element, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                        QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
         return d->style->drawComplexControl(control, option, painter, widget);

    QRect rect = option->rect;
    switch (control) {
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QString buttonType = widget->property("type").toString();
            QRect button, menuarea;
            button = subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state;
            if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver)) {
                    bflags &= ~State_Raised;
                }
            }

            State mflags = bflags;
            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    bflags |= State_Sunken;
                if (toolbutton->activeSubControls & SC_ToolButtonMenu)
                    mflags |= State_Sunken;
            }

            QStyleOption tool(0);
            tool.palette = toolbutton->palette;
            if (toolbutton->subControls & SC_ToolButton) {
                if (buttonType == "dockbutton") {
                    tool.rect = button;
                    tool.state = bflags;
                    drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                } else {  // paint status bar button style
                    if (bflags & State_Sunken || bflags & State_On)
                        drawCornerImage(d->buttonImage_pressed, painter, option->rect, 2, 2, 2, 2);
                    else if (bflags & State_Enabled) {
#ifndef Q_WS_MAC
                        if (bflags & State_MouseOver) {
                            drawCornerImage(d->buttonImage, painter, option->rect, 2, 2, 2, 2);
                            QColor shade(255, 255, 255, 50);
                            painter->fillRect(button.adjusted(1, 1, -1, -1), shade);
                        }
#endif
                    }

                }
            }

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                    fr.rect.adjust(0, 0, -pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                      toolbutton, widget), 0);
                QPen oldPen = painter->pen();
                QColor focusColor = StyleHelper::panelTextColor();
                focusColor.setAlpha(120);
                QPen outline(focusColor, 1);
                outline.setStyle(Qt::DotLine);
                painter->setPen(outline);
                QRect r = option->rect.adjusted(2, 2, -2, -2);
                painter->drawRect(r);
                painter->setPen(oldPen);
            }

            QStyleOptionToolButton label = *toolbutton;
            label.palette = panelPalette(option->palette);
            int fw = pixelMetric(PM_DefaultFrameWidth, option, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.state = mflags;
                tool.rect = menuarea.adjusted(1, 1, -1, -1);
                if (mflags & (State_Sunken | State_On | State_Raised)) {
                    painter->setPen(Qt::gray);
                    painter->drawLine(tool.rect.topLeft(), tool.rect.bottomLeft());
                    if (mflags & (State_Sunken)) {
                        QColor shade(0, 0, 0, 50);
                        painter->fillRect(tool.rect.adjusted(0, -1, 1, 1), shade);
                    }
#ifndef Q_WS_MAC
                    else if (mflags & (State_MouseOver)) {
                        QColor shade(255, 255, 255, 50);
                        painter->fillRect(tool.rect.adjusted(0, -1, 1, 1), shade);
                    }
#endif
                }
                tool.rect = tool.rect.adjusted(2, 2, -2, -2);
                drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu) {
                int arrowSize = 6;
                QRect ir = toolbutton->rect.adjusted(1, 1, -1, -1);
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.palette = panelPalette(option->palette);
                newBtn.rect = QRect(ir.right() - arrowSize - 1,
                                    ir.height() - arrowSize - 2, arrowSize, arrowSize);
                QWindowsStyle::drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        break;

    case CC_ComboBox:
        {
            painter->save();

            // Draw tool button
            drawPrimitive(PE_PanelButtonTool, option, painter, widget);

            // Draw arrow
            int menuButtonWidth = 16;
            bool reverse = option->direction == Qt::RightToLeft;
            int left = !reverse ? rect.right() - menuButtonWidth : rect.left();
            int right = !reverse ? rect.right() : rect.left() + menuButtonWidth;
            QRect arrowRect((left + right) / 2 - 5, rect.center().y() - 3, 9, 9);
            if (option->state & State_On)
                arrowRect.translate(d->style->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                    d->style->pixelMetric(PM_ButtonShiftVertical, option, widget));
            QStyleOption arrowOpt = *option;
            arrowOpt.rect = arrowRect;
            QPalette pal = option->palette;
            pal.setBrush(QPalette::All, QPalette::ButtonText, StyleHelper::panelTextColor());
            arrowOpt.palette = pal;

            QWindowsStyle::drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, painter, widget);

            painter->restore();
        }
        break;
    default:
        d->style->drawComplexControl(control, option, painter, widget);
        break;
    }
}
