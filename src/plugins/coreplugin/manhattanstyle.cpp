/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "manhattanstyle.h"

#include "styleanimator.h"

#include <coreplugin/coreconstants.h>

#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>

#include <utils/fancymainwindow.h>

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QToolButton>

// We define a currently unused state for indicating animations
const QStyle::State State_Animating = QStyle::State(0x00000040);

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
    if (!widget)
        return false;

    // Do not style dialogs or explicitly ignored widgets
    if ((widget->window()->windowFlags() & Qt::WindowType_Mask) == Qt::Dialog)
        return false;

    if (qobject_cast<const Utils::FancyMainWindow *>(widget))
        return true;

    if (qobject_cast<const QTabBar *>(widget))
        return styleEnabled(widget);

    const QWidget *p = widget;
    while (p) {
        if (qobject_cast<const QToolBar *>(p) ||
            qobject_cast<const QStatusBar *>(p) ||
            qobject_cast<const QMenuBar *>(p) ||
            p->property("panelwidget").toBool())
            return styleEnabled(widget);
        p = p->parentWidget();
    }
    return false;
}

// Consider making this a QStyle state
bool lightColored(const QWidget *widget)
{
    if (!widget)
        return false;

    // Don't style dialogs or explicitly ignored widgets
    if ((widget->window()->windowFlags() & Qt::WindowType_Mask) == Qt::Dialog)
        return false;

    const QWidget *p = widget;
    while (p) {
        if (p->property("lightColored").toBool())
            return true;
        p = p->parentWidget();
    }
    return false;
}

class ManhattanStylePrivate
{
public:
    explicit ManhattanStylePrivate();
    void init();

public:
    const QImage lineeditImage;
    const QImage lineeditImage_disabled;
    const QPixmap extButtonPixmap;
    const QPixmap closeButtonPixmap;
    StyleAnimator animator;
};

ManhattanStylePrivate::ManhattanStylePrivate() :
    lineeditImage(QLatin1String(":/core/images/inputfield.png")),
    lineeditImage_disabled(QLatin1String(":/core/images/inputfield_disabled.png")),
    extButtonPixmap(QLatin1String(":/core/images/extension.png")),
    closeButtonPixmap(QLatin1String(Core::Constants::ICON_CLOSE))
{
}

ManhattanStyle::ManhattanStyle(const QString &baseStyleName)
    : QProxyStyle(QStyleFactory::create(baseStyleName)),
    d(new ManhattanStylePrivate())
{
}

ManhattanStyle::~ManhattanStyle()
{
    delete d;
    d = 0;
}

QPixmap ManhattanStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const
{
    return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

QSize ManhattanStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                       const QSize &size, const QWidget *widget) const
{
    QSize newSize = QProxyStyle::sizeFromContents(type, option, size, widget);

    if (type == CT_Splitter && widget && widget->property("minisplitter").toBool())
        return QSize(1, 1);
    else if (type == CT_ComboBox && panelWidget(widget))
        newSize += QSize(14, 0);
    return newSize;
}

QRect ManhattanStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    return QProxyStyle::subElementRect(element, option, widget);
}

QRect ManhattanStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                     SubControl subControl, const QWidget *widget) const
{
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

QStyle::SubControl ManhattanStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                         const QPoint &pos, const QWidget *widget) const
{
    return QProxyStyle::hitTestComplexControl(control, option, pos, widget);
}

int ManhattanStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int retval = 0;
    retval = QProxyStyle::pixelMetric(metric, option, widget);
    switch (metric) {
    case PM_SplitterWidth:
        if (widget && widget->property("minisplitter").toBool())
            retval = 1;
        break;
    case PM_ToolBarIconSize:
        if (panelWidget(widget))
            retval = 16;
        break;
    case PM_DockWidgetHandleExtent:
    case PM_DockWidgetSeparatorExtent:
        return 1;
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
    return QProxyStyle::standardPalette();
}

void ManhattanStyle::polish(QApplication *app)
{
    return QProxyStyle::polish(app);
}

void ManhattanStyle::unpolish(QApplication *app)
{
    return QProxyStyle::unpolish(app);
}

QPalette panelPalette(const QPalette &oldPalette, bool lightColored = false)
{
    QColor color = Utils::StyleHelper::panelTextColor(lightColored);
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
    QProxyStyle::polish(widget);

    // OxygenStyle forces a rounded widget mask on toolbars and dock widgets
    if (baseStyle()->inherits("OxygenStyle") || baseStyle()->inherits("Oxygen::Style")) {
        if (qobject_cast<QToolBar*>(widget) || qobject_cast<QDockWidget*>(widget)) {
            widget->removeEventFilter(baseStyle());
            widget->setContentsMargins(0, 0, 0, 0);
        }
    }
    if (panelWidget(widget)) {

        // Oxygen and possibly other styles override this
        if (qobject_cast<QDockWidget*>(widget))
            widget->setContentsMargins(0, 0, 0, 0);

        widget->setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
        if (qobject_cast<QToolButton*>(widget)) {
            widget->setAttribute(Qt::WA_Hover);
            widget->setMaximumHeight(Utils::StyleHelper::navigationWidgetHeight() - 2);
        }
        else if (qobject_cast<QLineEdit*>(widget)) {
            widget->setAttribute(Qt::WA_Hover);
            widget->setMaximumHeight(Utils::StyleHelper::navigationWidgetHeight() - 2);
        }
        else if (qobject_cast<QLabel*>(widget))
            widget->setPalette(panelPalette(widget->palette()));
        else if (widget->property("panelwidget_singlerow").toBool())
            widget->setFixedHeight(Utils::StyleHelper::navigationWidgetHeight());
        else if (qobject_cast<QStatusBar*>(widget))
            widget->setFixedHeight(Utils::StyleHelper::navigationWidgetHeight() + 2);
        else if (qobject_cast<QComboBox*>(widget)) {
            widget->setMaximumHeight(Utils::StyleHelper::navigationWidgetHeight() - 2);
            widget->setAttribute(Qt::WA_Hover);
        }
    }
}

void ManhattanStyle::unpolish(QWidget *widget)
{
    QProxyStyle::unpolish(widget);
    if (panelWidget(widget)) {
        widget->setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
        if (qobject_cast<QTabBar*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
        else if (qobject_cast<QToolBar*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
        else if (qobject_cast<QComboBox*>(widget))
            widget->setAttribute(Qt::WA_Hover, false);
    }
}

void ManhattanStyle::polish(QPalette &pal)
{
    QProxyStyle::polish(pal);
}

QIcon ManhattanStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    QIcon icon;
    switch (standardIcon) {
    case QStyle::SP_TitleBarCloseButton:
    case QStyle::SP_ToolBarHorizontalExtensionButton:
        return QIcon(standardPixmap(standardIcon, option, widget));
    default:
        icon = baseStyle()->standardIcon(standardIcon, option, widget);
    }
    return icon;
}

QPixmap ManhattanStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                       const QWidget *widget) const
{
    if (widget && !panelWidget(widget))
        return QProxyStyle::standardPixmap(standardPixmap, opt, widget);

    QPixmap pixmap;
    switch (standardPixmap) {
    case QStyle::SP_ToolBarHorizontalExtensionButton:
        pixmap = d->extButtonPixmap;
        break;
    case QStyle::SP_TitleBarCloseButton:
        pixmap = d->closeButtonPixmap;
        break;
    default:
        pixmap = QProxyStyle::standardPixmap(standardPixmap, opt, widget);
        break;
    }
    return pixmap;
}

int ManhattanStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                              QStyleHintReturn *returnData) const
{
    int ret = QProxyStyle::styleHint(hint, option, widget, returnData);
    switch (hint) {
    // Make project explorer alternate rows all the way
    case QStyle::SH_ItemView_PaintAlternatingRowColorsForEmptyArea:
        if (widget && widget->property("AlternateEmpty").toBool())
            ret = true;
        break;
    case QStyle::SH_EtchDisabledText:
        if (panelWidget(widget))
            ret = false;
        break;
    case QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = true;
        break;
    default:
        break;
    }
    return ret;
}

void ManhattanStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
        return QProxyStyle::drawPrimitive(element, option, painter, widget);

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
            opt.state |= State_Animating;
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
            endOpt.state |= State_Animating;
            t->setStartImage(startImage);
            d->animator.startAnimation(t);
            endImage.fill(0);
            QPainter endPainter(&endImage);
            drawPrimitive(element, &endOpt, &endPainter, widget);
            t->setEndImage(endImage);
            if (oldState & State_MouseOver)
                t->setDuration(150);
            else
                t->setDuration(75);
            t->setStartTime(QTime::currentTime());
        }
    }

    switch (element) {
    case PE_IndicatorDockWidgetResizeHandle:
        painter->fillRect(option->rect, Utils::StyleHelper::borderColor());
        break;
    case PE_FrameDockWidget:
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    case PE_PanelLineEdit:
        {
            painter->save();

            // Fill the line edit background
            QRect filledRect = option->rect.adjusted(1, 1, -1, -1);
            painter->setBrushOrigin(filledRect.topLeft());
            painter->fillRect(filledRect, option->palette.base());

            if (option->state & State_Enabled)
                Utils::StyleHelper::drawCornerImage(d->lineeditImage, painter, option->rect, 5, 5, 5, 5);
            else
                Utils::StyleHelper::drawCornerImage(d->lineeditImage_disabled, painter, option->rect, 5, 5, 5, 5);

            if (option->state & State_HasFocus || option->state & State_MouseOver) {
                QColor hover = Utils::StyleHelper::baseColor();
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
                    QColor shade(0, 0, 0, 40);
                    painter->fillRect(rect, shade);
                    painter->drawLine(rect.topLeft() + QPoint(1, 0), rect.topRight() - QPoint(1, 0));
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                   // painter->drawLine(rect.bottomLeft()  + QPoint(1, 0), rect.bottomRight()  - QPoint(1, 0));
                    QColor highlight(255, 255, 255, 30);
                    painter->setPen(highlight);
                }
                else if (option->state & State_Enabled &&
                         option->state & State_MouseOver) {
                    QColor lighter(255, 255, 255, 37);
                    painter->fillRect(rect, lighter);
                }
                if (option->state & State_HasFocus && (option->state & State_KeyboardFocusChange)) {
                    QColor highlight = option->palette.highlight().color();
                    highlight.setAlphaF(0.4);
                    painter->setPen(QPen(highlight.lighter(), 1));
                    highlight.setAlphaF(0.3);
                    painter->setBrush(highlight);
                    painter->setRenderHint(QPainter::Antialiasing);
                    QRectF rect = option->rect;
                    rect.translate(0.5, 0.5);
                    painter->drawRoundedRect(rect.adjusted(2, 2, -3, -3), 2, 2);
                }
           }
        }
        break;

    case PE_PanelStatusBar:
        {
            painter->save();
            QLinearGradient grad = Utils::StyleHelper::statusBarGradient(rect);
            painter->fillRect(rect, grad);
            painter->setPen(QColor(255, 255, 255, 60));
            painter->drawLine(rect.topLeft() + QPoint(0,1),
                              rect.topRight()+ QPoint(0,1));
            painter->setPen(Utils::StyleHelper::borderColor().darker(110));
            painter->drawLine(rect.topLeft(), rect.topRight());
            painter->restore();
        }
        break;

    case PE_IndicatorToolBarSeparator:
        {
            QColor separatorColor = Utils::StyleHelper::borderColor();
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
            int x = option->rect.x() + (horizontal ? 2 : 6);
            int y = option->rect.y() + (horizontal ? 6 : 2);
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
            QColor dark = Utils::StyleHelper::borderColor();
            dark.setAlphaF(0.4);

            QColor light = Utils::StyleHelper::baseColor();
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
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft:
        {
            Utils::StyleHelper::drawArrow(element, painter, option);
        }
        break;

    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawControl(ControlElement element, const QStyleOption *option,
                                 QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
        return QProxyStyle::drawControl(element, option, painter, widget);

    switch (element) {
    case CE_Splitter:
        painter->fillRect(option->rect, Utils::StyleHelper::borderColor());
        break;

    case CE_TabBarTabShape:
        // Most styles draw a single dark outline. This looks rather ugly when combined with our
        // single pixel dark separator so we adjust the first tab to compensate for this

        if (const QStyleOptionTabV3 *tab = qstyleoption_cast<const QStyleOptionTabV3 *>(option)) {
            QStyleOptionTabV3 adjustedTab = *tab;
            if (tab->cornerWidgets == QStyleOptionTab::NoCornerWidgets && (
                    tab->position == QStyleOptionTab::Beginning ||
                    tab->position == QStyleOptionTab::OnlyOneTab))
            {
                if (option->direction == Qt::LeftToRight)
                    adjustedTab.rect = adjustedTab.rect.adjusted(-1, 0, 0, 0);
                else
                    adjustedTab.rect = adjustedTab.rect.adjusted(0, 0, 1 ,0);
            }
            QProxyStyle::drawControl(element, &adjustedTab, painter, widget);
            return;
        }
        break;

    case CE_MenuBarItem:
        painter->save();
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QColor highlightOutline = Utils::StyleHelper::borderColor().lighter(120);
            bool act = mbi->state & State_Sunken;
            bool dis = !(mbi->state & State_Enabled);
            Utils::StyleHelper::menuGradient(painter, option->rect, option->rect);
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect;
            QPalette pal = mbi->palette;
            pal.setBrush(QPalette::ButtonText, dis ? Qt::gray : Qt::black);
            item.palette = pal;
            QCommonStyle::drawControl(element, &item, painter, widget);
            QRect r = option->rect;

            if (act) {
                // Fill|
                QColor baseColor = Utils::StyleHelper::baseColor();
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
                painter->save();
                QRect editRect = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
                QPalette customPal = cb->palette;
                bool drawIcon = !(widget && widget->property("hideicon").toBool());

                if (!cb->currentIcon.isNull() && drawIcon) {
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

                    // Reserve some space for the down-arrow
                    editRect.adjust(0, 0, -13, 0);
                }

                QLatin1Char asterisk('*');
                int elideWidth = editRect.width();

                bool notElideAsterisk = widget && widget->property("notelideasterisk").toBool()
                                        && cb->currentText.endsWith(asterisk)
                                        && option->fontMetrics.width(cb->currentText) > elideWidth;

                QString text;
                if (notElideAsterisk) {
                    elideWidth -= option->fontMetrics.width(asterisk);
                    text = asterisk;
                }
                text.prepend(option->fontMetrics.elidedText(cb->currentText, Qt::ElideRight, elideWidth));

                if ((option->state & State_Enabled)) {
                    painter->setPen(QColor(0, 0, 0, 70));
                    painter->drawText(editRect.adjusted(1, 0, -1, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
                } else {
                    painter->setOpacity(0.8);
                }
                painter->setPen(Utils::StyleHelper::panelTextColor());
                painter->drawText(editRect.adjusted(1, 0, -1, 0), Qt::AlignLeft | Qt::AlignVCenter, text);

                painter->restore();
            } else {
                QProxyStyle::drawControl(element, option, painter, widget);
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
            Utils::StyleHelper::menuGradient(painter, option->rect, option->rect);
            painter->save();
            painter->setPen(Utils::StyleHelper::borderColor());
            painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
            painter->restore();
        }
        break;

    case CE_ToolBar:
        {
            QRect rect = option->rect;
            bool horizontal = option->state & State_Horizontal;
            rect = option->rect;

            // Map offset for global window gradient
            QPoint offset = widget->window()->mapToGlobal(option->rect.topLeft()) -
                            widget->mapToGlobal(option->rect.topLeft());
            QRect gradientSpan;
            if (widget)
                gradientSpan = QRect(offset, widget->window()->size());

            bool drawLightColored = lightColored(widget);
            if (horizontal)
                Utils::StyleHelper::horizontalGradient(painter, gradientSpan, rect, drawLightColored);
            else
                Utils::StyleHelper::verticalGradient(painter, gradientSpan, rect, drawLightColored);

            if (!drawLightColored)
                painter->setPen(Utils::StyleHelper::borderColor());
            else
                painter->setPen(QColor(0x888888));

            if (horizontal) {
                // Note: This is a hack to determine if the
                // toolbar should draw the top or bottom outline
                // (needed for the find toolbar for instance)
                QColor lighter(Utils::StyleHelper::sidebarHighlight());
                if (drawLightColored)
                    lighter = QColor(255, 255, 255, 180);
                if (widget && widget->property("topBorder").toBool()) {
                    painter->drawLine(rect.topLeft(), rect.topRight());
                    painter->setPen(lighter);
                    painter->drawLine(rect.topLeft() + QPoint(0, 1), rect.topRight() + QPoint(0, 1));
                } else {
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                    painter->setPen(lighter);
                    painter->drawLine(rect.topLeft(), rect.topRight());
                }
            } else {
                painter->drawLine(rect.topLeft(), rect.bottomLeft());
                painter->drawLine(rect.topRight(), rect.bottomRight());
            }
        }
        break;

    default:
        QProxyStyle::drawControl(element, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                        QPainter *painter, const QWidget *widget) const
{
    if (!panelWidget(widget))
         return     QProxyStyle::drawComplexControl(control, option, painter, widget);

    QRect rect = option->rect;
    switch (control) {
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            bool reverse = option->direction == Qt::RightToLeft;
            bool drawborder = (widget && widget->property("showborder").toBool());

            if (drawborder)
                drawButtonSeparator(painter, rect, reverse);

            QRect button, menuarea;
            button = subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state;
            if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver))
                    bflags &= ~State_Raised;
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
                tool.rect = button;
                tool.state = bflags;
                drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
            }

            QStyleOptionToolButton label = *toolbutton;

            label.palette = panelPalette(option->palette, lightColored(widget));
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
                    } else if (!Utils::HostOsInfo::isMacHost() && (mflags & State_MouseOver)) {
                        QColor shade(255, 255, 255, 50);
                        painter->fillRect(tool.rect.adjusted(0, -1, 1, 1), shade);
                    }
                }
                tool.rect = tool.rect.adjusted(2, 2, -2, -2);
                drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu
                       && !widget->property("noArrow").toBool()) {
                int arrowSize = 6;
                QRect ir = toolbutton->rect.adjusted(1, 1, -1, -1);
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.palette = panelPalette(option->palette);
                newBtn.rect = QRect(ir.right() - arrowSize - 1,
                                    ir.height() - arrowSize - 2, arrowSize, arrowSize);
                drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        break;

    case CC_ComboBox:
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            painter->save();
            bool isEmpty = cb->currentText.isEmpty() && cb->currentIcon.isNull();
            bool reverse = option->direction == Qt::RightToLeft;
            bool drawborder = !(widget && widget->property("hideborder").toBool());
            bool alignarrow = !(widget && widget->property("alignarrow").toBool());

            if (drawborder)
                drawButtonSeparator(painter, rect, reverse);

            QStyleOption toolbutton = *option;
            if (isEmpty)
                toolbutton.state &= ~(State_Enabled | State_Sunken);
            painter->save();
            if (drawborder)
                painter->setClipRect(toolbutton.rect.adjusted(0, 0, -2, 0));
            drawPrimitive(PE_PanelButtonTool, &toolbutton, painter, widget);
            painter->restore();
            // Draw arrow
            int menuButtonWidth = 12;
            int left = !reverse ? rect.right() - menuButtonWidth : rect.left();
            int right = !reverse ? rect.right() : rect.left() + menuButtonWidth;
            QRect arrowRect((left + right) / 2 + (reverse ? 6 : -6), rect.center().y() - 3, 9, 9);

            if (!alignarrow) {
                int labelwidth = option->fontMetrics.width(cb->currentText);
                if (reverse)
                    arrowRect.moveLeft(qMax(rect.width() - labelwidth - menuButtonWidth - 2, 4));
                else
                    arrowRect.moveLeft(qMin(labelwidth + menuButtonWidth - 2, rect.width() - menuButtonWidth - 4));
            }
            if (option->state & State_On)
                arrowRect.translate(QProxyStyle::pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                    QProxyStyle::pixelMetric(PM_ButtonShiftVertical, option, widget));

            QStyleOption arrowOpt = *option;
            arrowOpt.rect = arrowRect;
            if (isEmpty)
                arrowOpt.state &= ~(State_Enabled | State_Sunken);

            if (styleHint(SH_ComboBox_Popup, option, widget)) {
                arrowOpt.rect.translate(0, -3);
                drawPrimitive(PE_IndicatorArrowUp, &arrowOpt, painter, widget);
                arrowOpt.rect.translate(0, 6);
                drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, painter, widget);
            } else {
                drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, painter, widget);
            }

            painter->restore();
        }
        break;

    default:
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawButtonSeparator(QPainter *painter, const QRect &rect, bool reverse) const
{
    QLinearGradient grad(rect.topRight(), rect.bottomRight());
    grad.setColorAt(0, QColor(255, 255, 255, 20));
    grad.setColorAt(0.4, QColor(255, 255, 255, 60));
    grad.setColorAt(0.7, QColor(255, 255, 255, 50));
    grad.setColorAt(1, QColor(255, 255, 255, 40));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(rect.topRight(), rect.bottomRight());
    grad.setColorAt(0, QColor(0, 0, 0, 30));
    grad.setColorAt(0.4, QColor(0, 0, 0, 70));
    grad.setColorAt(0.7, QColor(0, 0, 0, 70));
    grad.setColorAt(1, QColor(0, 0, 0, 40));
    painter->setPen(QPen(grad, 0));
    if (!reverse)
       painter->drawLine(rect.topRight() - QPoint(1,0), rect.bottomRight() - QPoint(1,0));
    else
       painter->drawLine(rect.topLeft(), rect.bottomLeft());
 }
