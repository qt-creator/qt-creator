// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "studiostyle.h"
#include "studiostyle_p.h"

#include <utils/hostosinfo.h>
#include <utils/styleanimator.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QMenu>
#include <QPainter>
#include <QStyleOption>

#define ANIMATE_SCROLLBARS QT_CONFIG(animation)
using namespace Utils;
using namespace QmlDesigner;

namespace {

inline QColor studioTextColor(bool enabled,
                       bool active,
                       bool checked)
{
    Theme::Color themePenColorId =
            enabled ? (active
                       ? (checked ? Theme::DSsubPanelBackground
                                  : Theme::DSpanelBackground)
                       : Theme::DStextColor)
                    : Theme::DStextColorDisabled;

    return creatorTheme()->color(themePenColorId);
}

inline QColor studioButtonBgColor(bool enabled,
                           bool hovered,
                           bool pressed,
                           bool checked)
{
    Theme::Color themePenColorId =
            enabled
            ? (pressed
               ? (checked
                  ? Theme::DSinteractionHover
                  : Theme::DScontrolBackgroundGlobalHover)
               : (hovered
                  ? (checked
                     ? Theme::DSinteractionHover
                     : Theme::DScontrolBackgroundHover)
                  : Theme::DScontrolBackground)
                 )
            : Theme::DScontrolBackgroundDisabled;

    return creatorTheme()->color(themePenColorId);
}

inline QColor studioButtonOutlineColor(bool enabled,
                                [[maybe_unused]] bool hovered,
                                bool pressed,
                                [[maybe_unused]] bool checked)
{

    Theme::Color themePenColorId =
            enabled
            ? (pressed
               ? Theme::DScontrolOutlineInteraction
               : Theme::DScontrolOutline)
            : Theme::DScontrolOutlineDisabled;

    return creatorTheme()->color(themePenColorId);
}

inline bool anyParentsFocused(const QWidget *widget)
{
    const QWidget *p = widget;
    while (p) {
        if (p->property("focused").toBool())
            return true;
        p = p->parentWidget();
    }
    return false;
}

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
bool isQmlEditorMenu(const QWidget *widget)
{
    const QMenu *menu = qobject_cast<const QMenu *> (widget);
    if (!menu)
        return false;

    const QWidget *p = widget;
    while (p) {
        if (p->property("qmlEditorMenu").toBool())
            return styleEnabled(widget);
        p = p->parentWidget();
    }

    return false;
}

inline QPixmap getPixmapFromIcon(
    const QIcon &icon, const QSize &size, bool enabled, bool active, bool checked)
{
    QIcon::Mode mode = enabled ? ((active) ? QIcon::Active : QIcon::Normal) : QIcon::Disabled;
    QIcon::State state = (checked) ? QIcon::On : QIcon::Off;
    return icon.pixmap(size, mode, state);
}

inline QRect expandScrollRect(const QRect &ref,
                              const qreal &factor,
                              const Qt::Orientation &orientation)
{
    if (qFuzzyCompare(factor, 1))
        return ref;

    if (orientation == Qt::Horizontal) {
        qreal newExp = ref.height() * factor;
        qreal newDiff = ref.height() - newExp;
        return ref.adjusted(0, newDiff, 0, 0);
    } else {
        qreal newExp = ref.width() * factor;
        qreal newDiff = ref.width() - newExp;
        return ref.adjusted(newDiff, 0, 0, 0);
    }
}

} // namespace

StudioStyle::StudioStyle(QStyle *style)
    : QProxyStyle(style)
    , d(new StudioStylePrivate(this))
{
}

StudioStyle::StudioStyle(const QString &key)
    : QProxyStyle(key)
    , d(new StudioStylePrivate(this))
{
}

StudioStyle::~StudioStyle()
{
    delete d;
}

void StudioStyle::drawPrimitive(
        PrimitiveElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget) const
{
    switch (element) {
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
        if (const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QStyleOptionMenuItem item = *mbi;
            item.palette = QPalette(Qt::white);
            StyleHelper::drawMinimalArrow(element, painter, &item);
        } else {
            Super::drawPrimitive(element, option, painter, widget);
        }
        break;

    case PE_IndicatorArrowRight:
        drawQmlEditorIcon(element, option, "cascadeIconRight", painter, widget);
        break;

    case PE_IndicatorArrowLeft:
        drawQmlEditorIcon(element, option, "cascadeIconLeft", painter, widget);
        break;

    case PE_IndicatorMenuCheckMark:
        drawQmlEditorIcon(element, option, "tickIcon", painter, widget);
        break;

    case PE_FrameMenu:
    case PE_PanelMenu:
        if (isQmlEditorMenu(widget))
            painter->fillRect(option->rect, creatorTheme()->color(Theme::DSsubPanelBackground));
        else
            Super::drawPrimitive(element, option, painter, widget);
        break;

    case PE_PanelButtonCommand:
        if (!isQmlEditorMenu(widget))
            Super::drawPrimitive(element, option, painter, widget);
        break;
    case PE_FrameDefaultButton: {
        if (const auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            bool enabled = button->state & QStyle::State_Enabled;
            bool hovered = enabled && button->state & QStyle::State_MouseOver;
            bool checked = button->state & QStyle::State_On;
            bool pressed = enabled && button->state & QStyle::State_Sunken;
            QColor btnColor = studioButtonBgColor(enabled, hovered, pressed, checked);
            QColor penColor = studioButtonOutlineColor(enabled, hovered, pressed, checked);
            penColor.setAlpha(50);

            painter->save();
            painter->setPen(QPen(penColor, 1));
            if (pressed) {
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 5, 5);
                painter->setPen(Qt::NoPen);
                painter->setBrush(btnColor);
                painter->drawRoundedRect(option->rect.adjusted(1, 1, 0, 0), 5, 5);
            } else {
                painter->setPen(Qt::NoPen);
                painter->setBrush(btnColor);
                painter->drawRoundedRect(option->rect, 5, 5);
            }

            painter->restore();
        }
    } break;

    case PE_IndicatorToolBarSeparator: {
        bool horizontal = option->state & State_Horizontal;
        int thickness = pixelMetric(PM_ToolBarSeparatorExtent, option, widget);
        QRect colorRect;
        if (horizontal) {
            colorRect = {option->rect.center().x() - thickness / 2,
                         option->rect.top() + 2,
                         thickness,
                         option->rect.height() - 4};
        } else {
            colorRect = {option->rect.left() + 2,
                         option->rect.center().y() - thickness / 2,
                         option->rect.width() - 4,
                         thickness};
        }

        // The separator color is currently the same as toolbar bg
        painter->fillRect(colorRect, creatorTheme()->color(Theme::DStoolbarBackground));
    } break;

    default: {
        Super::drawPrimitive(element, option, painter, widget);
        break;
    }
    }
}

void StudioStyle::drawControl(
        ControlElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget) const
{
    switch (element) {
    case CE_MenuItem:
        if (const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (!isQmlEditorMenu(widget)) {
                Super::drawControl(element, option, painter, widget);
                break;
            }
            painter->save();
            const int iconHeight = pixelMetric(QStyle::PM_SmallIconSize, option, widget);
            const int horizontalSpacing = pixelMetric(QStyle::PM_LayoutHorizontalSpacing, option, widget);
            const int iconWidth = iconHeight;
            const bool isActive = mbi->state & State_Selected;
            const bool isDisabled = !(mbi->state & State_Enabled);
            const bool isCheckable = mbi->checkType != QStyleOptionMenuItem::NotCheckable;
            const bool isChecked = isCheckable ? mbi->checked : false;
            int startMargin = pixelMetric(QStyle::PM_LayoutLeftMargin, option, widget);
            int endMargin = pixelMetric(QStyle::PM_LayoutRightMargin, option, widget);
            int forwardX = 0;

            if (option->direction == Qt::RightToLeft)
                std::swap(startMargin, endMargin);

            QStyleOptionMenuItem item = *mbi;

            if (isActive) {
                painter->fillRect(item.rect, creatorTheme()->color(Theme::DSinteraction));
            }
            forwardX += startMargin;

            if (item.menuItemType == QStyleOptionMenuItem::Separator) {
                int commonHeight = item.rect.center().y();
                int additionalMargin = forwardX;
                QLineF separatorLine (item.rect.left() + additionalMargin,
                                      commonHeight,
                                      item.rect.right() - additionalMargin,
                                      commonHeight);

                painter->setPen(creatorTheme()->color(Theme::DSstateSeparatorColor));
                painter->drawLine(separatorLine);
                item.text.clear();
                painter->restore();
                return;
            }

            QPixmap iconPixmap;
            QIcon::Mode mode = isDisabled ? QIcon::Disabled : ((isActive) ? QIcon::Active : QIcon::Normal);
            QIcon::State state = isChecked ? QIcon::On : QIcon::Off;
            QColor themePenColor = studioTextColor(!isDisabled, isActive, isChecked);

            if (!item.icon.isNull()) {
                iconPixmap = item.icon.pixmap(QSize(iconHeight, iconHeight), mode, state);
            } else if (isCheckable) {
                iconPixmap = QPixmap(iconHeight, iconHeight);
                iconPixmap.fill(Qt::transparent);

                if (item.checked) {
                    QStyleOptionMenuItem so = item;
                    so.rect = iconPixmap.rect();
                    QPainter dPainter(&iconPixmap);
                    dPainter.setPen(themePenColor);
                    drawPrimitive(PE_IndicatorMenuCheckMark, &so, &dPainter, widget);
                }
            }

            if (!iconPixmap.isNull()) {
                QRect vCheckRect = visualRect(item.direction,
                                              item.rect,
                                              QRect(item.rect.x() + forwardX,
                                                    item.rect.y(),
                                                    iconWidth,
                                                    item.rect.height()));

                QRect pmr(QPoint(0, 0), iconPixmap.deviceIndependentSize().toSize());
                pmr.moveCenter(vCheckRect.center());
                painter->setPen(themePenColor);
                painter->drawPixmap(pmr.topLeft(), iconPixmap);

                item.checkType = QStyleOptionMenuItem::NotCheckable;
                item.checked = false;
                item.icon = {};
            }
            if (item.menuHasCheckableItems || item.maxIconWidth > 0) {
                forwardX += iconWidth + horizontalSpacing;
            }

            QString shortcutText;
            int tabIndex = item.text.indexOf("\t");
            if (tabIndex > -1) {
                shortcutText = item.text.mid(tabIndex + 1);
                item.text = item.text.left(tabIndex);
            }

            if (item.text.size()) {
                painter->save();

                QRect vTextRect = visualRect(item.direction,
                                             item.rect,
                                             item.rect.adjusted(forwardX, 0 , 0 , 0));

                Qt::Alignment alignmentFlags = item.direction == Qt::LeftToRight ? Qt::AlignLeft
                                                                                 : Qt::AlignRight;
                alignmentFlags |= Qt::AlignVCenter;

                int textFlags = Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, &item, widget))
                    textFlags |= Qt::TextHideMnemonic;
                textFlags |= alignmentFlags;

                painter->setPen(themePenColor);
                painter->drawText(vTextRect, textFlags, item.text);
                painter->restore();
            }

            if (item.menuItemType == QStyleOptionMenuItem::SubMenu) {
                PrimitiveElement dropDirElement = item.direction == Qt::LeftToRight
                        ? PE_IndicatorArrowRight
                        : PE_IndicatorArrowLeft;

                QSize elSize(iconHeight, iconHeight);
                int xOffset = iconHeight + endMargin;
                int yOffset = (item.rect.height() - iconHeight) / 2;
                QRect dropRect(item.rect.topRight(), elSize);
                dropRect.adjust(-xOffset, yOffset, -xOffset, yOffset);

                QStyleOptionMenuItem so = item;
                so.rect = visualRect(item.direction,
                                     item.rect,
                                     dropRect);

                drawPrimitive(dropDirElement, &so, painter, widget);
            } else if (!shortcutText.isEmpty()) {
                QPixmap pix = StudioShortcut(&item, shortcutText).getPixmap();

                if (pix.width()) {
                    int xOffset = pix.width() + (iconHeight / 2) + endMargin;
                    QRect shortcutRect = item.rect.translated({item.rect.width() - xOffset, 0});
                    shortcutRect.setSize({pix.width(), item.rect.height()});
                    shortcutRect = visualRect(item.direction,
                                              item.rect,
                                              shortcutRect);
                    drawItemPixmap(painter,
                                   shortcutRect,
                                   Qt::AlignRight | Qt::AlignVCenter,
                                   pix);
                }
            }
            painter->restore();
        }
        break;

    case CE_MenuEmptyArea:
        if (isQmlEditorMenu(widget))
            drawPrimitive(PE_PanelMenu, option, painter, widget);
        else
            Super::drawControl(element, option, painter, widget);
        break;

    default:
        Super::drawControl(element, option, painter, widget);
        break;
    }
}

void StudioStyle::drawComplexControl(
        ComplexControl control,
        const QStyleOptionComplex *option,
        QPainter *painter,
        const QWidget *widget) const
{
    switch (control) {
    case CC_Slider: {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);

            bool horizontal = slider->orientation == Qt::Horizontal;
            bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
            bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
            bool enabled = option->state & QStyle::State_Enabled;
            bool grooveHover = slider->activeSubControls & SC_SliderGroove;
            bool handleHover = slider->activeSubControls & SC_SliderHandle;
            bool interaction = option->state & State_Sunken;
            bool activeFocus = option->state & State_HasFocus && option->state & State_KeyboardFocusChange;

            int sliderPaintingOffset = horizontal
                    ? handle.center().x()
                    : handle.center().y();

            int borderRadius = 4;

            painter->save();
            painter->setRenderHint(QPainter::RenderHint::Antialiasing);

            Theme::Color themeframeColor = enabled
                    ? interaction
                      ? Theme::DSstateControlBackgroundColor_hover // Pressed
                      : grooveHover
                          ? Theme::DSstateSeparatorColor // GrooveHover
                          : Theme::DSpopupBackground // Idle
                    : Theme::DSpopupBackground; // Disabled

            QColor frameColor = creatorTheme()->color(themeframeColor);

            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                Theme::Color themeBgPlusColor = enabled
                        ? interaction
                          ? Theme::DSstateControlBackgroundColor_hover // Pressed
                          : grooveHover
                              ? Theme::DSstateSeparatorColor // GrooveHover
                              : Theme::DSstateControlBackgroundColor_hover // Idle should be the same as pressed
                        : Theme::DStoolbarBackground; // Disabled

                Theme::Color themeBgMinusColor = Theme::DSpopupBackground;

                QRect minusRect(groove);
                QRect plusRect(groove);

                if (horizontal) {
                    if (slider->upsideDown) {
                        minusRect.setLeft(sliderPaintingOffset);
                        plusRect.setRight(sliderPaintingOffset);
                    } else {
                        minusRect.setRight(sliderPaintingOffset);
                        plusRect.setLeft(sliderPaintingOffset);
                    }
                } else {
                    if (slider->upsideDown) {
                        minusRect.setBottom(sliderPaintingOffset);
                        plusRect.setTop(sliderPaintingOffset);
                    } else {
                        minusRect.setTop(sliderPaintingOffset);
                        plusRect.setBottom(sliderPaintingOffset);
                    }
                }

                painter->save();
                painter->setPen(Qt::NoPen);
                painter->setBrush(creatorTheme()->color(themeBgPlusColor));
                painter->drawRoundedRect(plusRect, borderRadius, borderRadius);
                painter->setBrush(creatorTheme()->color(themeBgMinusColor));
                painter->drawRoundedRect(minusRect, borderRadius, borderRadius);
                painter->restore();
            }

            if (option->subControls & SC_SliderTickmarks) {
                Theme::Color tickPen = enabled
                        ? activeFocus
                          ? Theme::DSstateBackgroundColor_hover
                          : Theme::DSBackgroundColorAlternate
                        : Theme::DScontrolBackgroundDisabled;

                painter->setBrush(Qt::NoBrush);
                painter->setPen(creatorTheme()->color(tickPen));
                int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                              0, available) < 3)
                        interval = slider->pageStep;
                }
                if (interval <= 0)
                    interval = 1;

                int v = slider->minimum;
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int pos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                      v_, (horizontal
                                                           ? slider->rect.width()
                                                           : slider->rect.height()) - len,
                                                      slider->upsideDown) + len / 2;
                    int extra = 2 - ((v_ == slider->minimum || v_ == slider->maximum) ? 1 : 0);

                    if (horizontal) {
                        if (ticksAbove) {
                            painter->drawLine(pos, slider->rect.top() + extra,
                                              pos, slider->rect.top() + tickSize);
                        }
                        if (ticksBelow) {
                            painter->drawLine(pos, slider->rect.bottom() - extra,
                                              pos, slider->rect.bottom() - tickSize);
                        }
                    } else {
                        if (ticksAbove) {
                            painter->drawLine(slider->rect.left() + extra, pos,
                                              slider->rect.left() + tickSize, pos);
                        }
                        if (ticksBelow) {
                            painter->drawLine(slider->rect.right() - extra, pos,
                                              slider->rect.right() - tickSize, pos);
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
            }

            // draw handle
            if (option->subControls & SC_SliderHandle) {
                Theme::Color handleColor = enabled
                        ? interaction
                            ? Theme::DSinteraction  // Interaction
                            : grooveHover || handleHover
                              ? Theme::DStabActiveText // Hover
                              : Theme::PalettePlaceholderText // Idle
                        : Theme::DStoolbarIcon_blocked; // Disabled

                int halfSliderThickness = horizontal
                        ? handle.width() / 2
                        : handle.height() / 2;
                painter->setBrush(creatorTheme()->color(handleColor));
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(handle,
                                        halfSliderThickness,
                                        halfSliderThickness);
            }

            if (groove.isValid()) {
                int borderWidth = pixelMetric(QStyle::PM_DefaultFrameWidth, option, widget);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(frameColor, borderWidth));
                painter->drawRoundedRect(groove, borderRadius, borderRadius);
            }
            painter->restore();
        }
    } break;
    case CC_ComboBox: {
        painter->fillRect(option->rect, standardPalette().brush(QPalette::ColorRole::Base));
        Super::drawComplexControl(control, option, painter, widget);
    } break;

#if QT_CONFIG(slider)
    case CC_ScrollBar: {
        painter->save();
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(
                option)) {
            bool wasActive = false;
            bool wasAdjacent = false;
            bool isFocused = anyParentsFocused(widget);
            bool isAdjacent = false;
            qreal scaleCoFactor = 1.0;
            QObject *styleObject = option->styleObject;
            bool hasTransientStyle = proxy()->styleHint(SH_ScrollBar_Transient, option, widget);
            if (styleObject && hasTransientStyle) {
#if ANIMATE_SCROLLBARS
                qreal opacity = 0.0;
                bool shouldExpand = false;
                const qreal minExpandScale = 0.7;
                const qreal maxExpandScale = 1.0;
#endif
                isAdjacent = styleObject->property("adjacentScroll").toBool();
                int oldPos = styleObject->property("_qdss_stylepos").toInt();
                int oldMin = styleObject->property("_qdss_stylemin").toInt();
                int oldMax = styleObject->property("_qdss_stylemax").toInt();
                QRect oldRect = styleObject->property("_qdss_stylerect").toRect();
                QStyle::State oldState = static_cast<QStyle::State>(
                    qvariant_cast<QStyle::State::Int>(styleObject->property("_qdss_stylestate")));
                uint oldActiveControls = styleObject->property("_qdss_stylecontrols").toUInt();
                bool oldFocus = styleObject->property("_qdss_focused").toBool();
                bool oldAdjacent = styleObject->property("_qdss_adjacentScroll").toBool();
                // a scrollbar is transient when the scrollbar itself and
                // its sibling are both inactive (ie. not pressed/hovered/moved)
                bool transient = !option->activeSubControls && !(option->state & State_On);
                if (!transient || oldPos != scrollBar->sliderPosition
                    || oldMin != scrollBar->minimum || oldMax != scrollBar->maximum
                    || oldRect != scrollBar->rect || oldState != scrollBar->state
                    || oldActiveControls != scrollBar->activeSubControls || oldFocus != isFocused
                    || oldAdjacent != isAdjacent) {
                    styleObject->setProperty("_qdss_stylepos", scrollBar->sliderPosition);
                    styleObject->setProperty("_qdss_stylemin", scrollBar->minimum);
                    styleObject->setProperty("_qdss_stylemax", scrollBar->maximum);
                    styleObject->setProperty("_qdss_stylerect", scrollBar->rect);
                    styleObject->setProperty("_qdss_stylestate",
                                             static_cast<QStyle::State::Int>(scrollBar->state));
                    styleObject->setProperty("_qdss_stylecontrols",
                                             static_cast<uint>(scrollBar->activeSubControls));
                    styleObject->setProperty("_qdss_focused", isFocused);
                    styleObject->setProperty("_qdss_adjacentScroll", isAdjacent);
#if ANIMATE_SCROLLBARS
                    // if the scrollbar is transient or its attributes, geometry or
                    // state has changed, the opacity is reset back to 100% opaque
                    opacity = 1.0;
                    QScrollbarStyleAnimation *anim = qobject_cast<QScrollbarStyleAnimation *>(
                        d->animation(styleObject));
                    if (transient) {
                        if (anim && anim->mode() != QScrollbarStyleAnimation::Deactivating) {
                            d->stopAnimation(styleObject);
                            anim = nullptr;
                        }
                        if (!anim) {
                            anim = new QScrollbarStyleAnimation(QScrollbarStyleAnimation::Deactivating,
                                                                styleObject);
                            d->startAnimation(anim);
                        } else if (anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                            // the scrollbar was already fading out while the
                            // state changed -> restart the fade out animation
                            anim->setCurrentTime(0);
                        }
                    } else if (anim && anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                        d->stopAnimation(styleObject);
                    }
#endif // animation
                }
#if ANIMATE_SCROLLBARS
                QScrollbarStyleAnimation *anim = qobject_cast<QScrollbarStyleAnimation *>(
                    d->animation(styleObject));
                if (anim && anim->mode() == QScrollbarStyleAnimation::Deactivating) {
                    // once a scrollbar was active (hovered/pressed), it retains
                    // the active look even if it's no longer active while fading out
                    if (oldActiveControls)
                        anim->setActive(true);

                    if (oldAdjacent)
                        anim->setAdjacent(true);

                    wasActive = anim->wasActive();
                    wasAdjacent = anim->wasAdjacent();
                    opacity = anim->currentValue();
                }
                shouldExpand = (option->activeSubControls || wasActive);
                if (shouldExpand) {
                    if (!anim && !oldActiveControls) {
                        // Start expand animation only once and when entering
                        anim = new QScrollbarStyleAnimation(QScrollbarStyleAnimation::Activating,
                                                            styleObject);
                        d->startAnimation(anim);
                    }
                    if (anim && anim->mode() == QScrollbarStyleAnimation::Activating) {
                        scaleCoFactor = (1.0 - anim->currentValue()) * minExpandScale
                                        + anim->currentValue() * maxExpandScale;
                    } else {
                        // Keep expanded state after the animation ends, and when fading out
                        scaleCoFactor = maxExpandScale;
                    }
                }
                painter->setOpacity(opacity);
#endif // animation
            }
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool sunken = scrollBar->state & State_Sunken;
            QRect scrollBarSubLine = proxy()->subControlRect(control,
                                                             scrollBar,
                                                             SC_ScrollBarSubLine,
                                                             widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control,
                                                             scrollBar,
                                                             SC_ScrollBarAddLine,
                                                             widget);
            QRect scrollBarSlider = proxy()->subControlRect(control,
                                                            scrollBar,
                                                            SC_ScrollBarSlider,
                                                            widget);
            QRect scrollBarGroove = proxy()->subControlRect(control,
                                                            scrollBar,
                                                            SC_ScrollBarGroove,
                                                            widget);
            QRect rect = option->rect;

            QColor alphaOutline = StyleHelper::borderColor();
            alphaOutline.setAlpha(180);
            QColor arrowColor = option->palette.windowText().color();
            arrowColor.setAlpha(160);

            bool enabled = scrollBar->state & QStyle::State_Enabled;
            bool hovered = enabled && scrollBar->state & QStyle::State_MouseOver;

            QColor buttonColor = creatorTheme()->color(hovered ? Theme::DSscrollBarHandle
                                                               : Theme::DSscrollBarHandle_idle);
            QColor gradientStartColor = buttonColor.lighter(118);
            QColor gradientStopColor = buttonColor;
            if (hasTransientStyle) {
                rect = expandScrollRect(rect, scaleCoFactor, scrollBar->orientation);
                scrollBarSlider = expandScrollRect(scrollBarSlider,
                                                   scaleCoFactor,
                                                   scrollBar->orientation);
                scrollBarGroove = expandScrollRect(scrollBarGroove,
                                                   scaleCoFactor,
                                                   scrollBar->orientation);
            }
            // Paint groove
            if ((!hasTransientStyle || scrollBar->activeSubControls || wasActive || isAdjacent
                 || wasAdjacent)
                && scrollBar->subControls & SC_ScrollBarGroove) {
                painter->save();
                painter->setPen(Qt::NoPen);
                if (hasTransientStyle) {
                    QColor brushColor(creatorTheme()->color(Theme::DSscrollBarTrack));
                    brushColor.setAlpha(0.3 * 255);
                    painter->setBrush(QBrush(brushColor));
                    painter->drawRoundedRect(scrollBarGroove, 4, 4);
                } else {
                    painter->setBrush(QBrush(creatorTheme()->color(Theme::DSscrollBarTrack)));
                    painter->drawRect(rect);
                }
                painter->restore();
            }
            QRect pixmapRect = scrollBarSlider;
            QLinearGradient gradient(pixmapRect.center().x(),
                                     pixmapRect.top(),
                                     pixmapRect.center().x(),
                                     pixmapRect.bottom());
            if (!horizontal)
                gradient = QLinearGradient(pixmapRect.left(),
                                           pixmapRect.center().y(),
                                           pixmapRect.right(),
                                           pixmapRect.center().y());
            QLinearGradient highlightedGradient = gradient;
            QColor midColor2 = StyleHelper::mergedColors(gradientStartColor, gradientStopColor, 40);
            gradient.setColorAt(0, buttonColor.lighter(108));
            gradient.setColorAt(1, buttonColor);
            QColor innerContrastLine = StyleHelper::highlightColor();
            highlightedGradient.setColorAt(0, gradientStartColor.darker(102));
            highlightedGradient.setColorAt(1, gradientStopColor.lighter(102));
            // Paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                if (hasTransientStyle) {
                    QRect rect = scrollBarSlider;
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(highlightedGradient);
                    int r = qMin(rect.width(), rect.height()) / 2;
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->drawRoundedRect(rect, r, r);
                    painter->restore();
                } else {
                    QRect pixmapRect = scrollBarSlider;
                    painter->setPen(QPen(alphaOutline));
                    if (option->state & State_Sunken
                        && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(midColor2);
                    else if (option->state & State_MouseOver
                             && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(highlightedGradient);
                    else
                        painter->setBrush(gradient);
                    painter->drawRect(pixmapRect.adjusted(horizontal ? -1 : 0,
                                                          horizontal ? 0 : -1,
                                                          horizontal ? 0 : 1,
                                                          horizontal ? 1 : 0));
                    painter->setPen(innerContrastLine);
                    painter->drawRect(
                        scrollBarSlider.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));
                }
            }
            // The SubLine (up/left) buttons
            if (!hasTransientStyle && scrollBar->subControls & SC_ScrollBarSubLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarSubLine))
                    painter->setBrush(highlightedGradient);
                else
                    painter->setBrush(gradient);
                painter->setPen(Qt::NoPen);
                painter->drawRect(
                    scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::RightToLeft) {
                        pixmapRect.setLeft(scrollBarSubLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarSubLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setBottom(scrollBarSubLine.bottom());
                    painter->drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());
                }
                QRect upRect = scrollBarSubLine.adjusted(horizontal ? 0 : 1,
                                                         horizontal ? 1 : 0,
                                                         horizontal ? -2 : -1,
                                                         horizontal ? -1 : -2);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(innerContrastLine);
                painter->drawRect(upRect);
                // Arrows
                PrimitiveElement arrowType = PE_IndicatorArrowUp;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? PE_IndicatorArrowLeft
                                                                     : PE_IndicatorArrowRight;
                StyleHelper::drawArrow(arrowType, painter, option);
            }
            // The AddLine (down/right) button
            if (!hasTransientStyle && scrollBar->subControls & SC_ScrollBarAddLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarAddLine))
                    painter->setBrush(midColor2);
                else
                    painter->setBrush(gradient);
                painter->setPen(Qt::NoPen);
                painter->drawRect(
                    scrollBarAddLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline, 1));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::LeftToRight) {
                        pixmapRect.setLeft(scrollBarAddLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarAddLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setTop(scrollBarAddLine.top());
                    painter->drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
                }
                QRect downRect = scrollBarAddLine.adjusted(1, 1, -1, -1);
                painter->setPen(innerContrastLine);
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(downRect);
                PrimitiveElement arrowType = PE_IndicatorArrowDown;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? PE_IndicatorArrowRight
                                                                     : PE_IndicatorArrowLeft;
                StyleHelper::drawArrow(arrowType, painter, option);
            }
        }
        painter->restore();
    } break;
#endif // QT_CONFIG(slider)
    default:
        Super::drawComplexControl(control, option, painter, widget);
        break;
    }
}

QSize StudioStyle::sizeFromContents(
        ContentsType type,
        const QStyleOption *option,
        const QSize &size,
        const QWidget *widget) const
{
    QSize newSize;

    switch (type) {
    case CT_MenuItem: {
        if (const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (!isQmlEditorMenu(widget)) {
                newSize = Super::sizeFromContents(type, option, size, widget);
                break;
            }

            const int leftMargin = pixelMetric(
                        QStyle::PM_LayoutLeftMargin,
                        option, widget);
            const int rightMargin = pixelMetric(
                        QStyle::PM_LayoutRightMargin,
                        option, widget);
            const int horizontalSpacing = pixelMetric(
                        QStyle::PM_LayoutHorizontalSpacing,
                        option, widget);
            const int iconHeight = pixelMetric(
                        QStyle::PM_SmallIconSize, option, widget) + horizontalSpacing;
            int width = leftMargin + rightMargin;
            if (mbi->menuHasCheckableItems || mbi->maxIconWidth)
                width += iconHeight + horizontalSpacing;

            if (!mbi->text.isEmpty()) {
                QString itemText = mbi->text;
                QString shortcutText;
                int tabIndex = itemText.indexOf("\t");
                if (tabIndex > -1) {
                    shortcutText = itemText.mid(tabIndex + 1);
                    itemText = itemText.left(tabIndex);
                }

                if (itemText.size())
                    width += option->fontMetrics.boundingRect(itemText).width() + horizontalSpacing;

                if (shortcutText.size()) {
                    QSize shortcutSize = StudioShortcut(mbi, shortcutText).getSize();
                    width += shortcutSize.width() + 2 * horizontalSpacing;
                }
            }

            if (mbi->menuItemType == QStyleOptionMenuItem::SubMenu)
                width += iconHeight + horizontalSpacing;

            newSize.setWidth(width);

            switch (mbi->menuItemType) {
            case QStyleOptionMenuItem::Normal:
            case QStyleOptionMenuItem::DefaultItem:
            case QStyleOptionMenuItem::SubMenu:
                newSize.setHeight(19);
                break;
            default:
                newSize.setHeight(9);
                break;
            }
        }
    } break;
    default:
        newSize = Super::sizeFromContents(type, option, size, widget);
        break;
    }

    return newSize;
}

QRect StudioStyle::subControlRect(
        ComplexControl control,
        const QStyleOptionComplex *option,
        SubControl subControl,
        const QWidget *widget) const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 5)
    // Workaround for QTBUG-101581, can be removed when building with Qt 6.2.5 or higher
    if (control == CC_ScrollBar) {
        const auto scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (scrollbar && qint64(scrollbar->maximum) - scrollbar->minimum > INT_MAX)
            return QRect(); // breaks the scrollbar, but avoids the crash
    }
#endif

    switch (control) {
    case CC_Slider: {
        if (const auto slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            switch (subControl) {
            case SubControl::SC_SliderGroove:
                return slider->rect;
            case SubControl::SC_SliderHandle: {
                QRect retval = Super::subControlRect(control, option, subControl, widget);
                return (slider->orientation == Qt::Horizontal) ? retval.adjusted(0, 1, 0, 0)
                                                               : retval.adjusted(1, 0, 0, 0);
            } break;
            default:
                break;
            }
        }
    } break;
    case CC_ScrollBar: {
        if (!styleHint(SH_ScrollBar_Transient, option, widget))
            break;

        if (const auto scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect newRect = Super::subControlRect(control, option, subControl, widget);
            if (Utils::HostOsInfo::isMacHost()) {
                if (scrollbar->orientation == Qt::Horizontal) {
                    const int halfThickness = newRect.height() / 2;
                    newRect.adjust(0, halfThickness, 0, 0);
                } else {
                    const int halfThickness = newRect.width() / 2;
                    newRect.adjust(halfThickness, 0, 0, 0);
                }
            }
            if (subControl == SC_ScrollBarSlider) {
                bool hasGroove
                    = (scrollbar->activeSubControls.testFlag(SC_SliderGroove)
                       || (scrollbar->styleObject
                           && scrollbar->styleObject->property("adjacentScrollBar").toBool()))
                      && scrollbar->subControls.testFlag(SC_ScrollBarGroove);
                bool interacted = scrollbar->activeSubControls.testFlag(SC_ScrollBarSlider);

                if (hasGroove || interacted)
                    return newRect;

                if (scrollbar->orientation == Qt::Horizontal)
                    newRect.adjust(0, 1, 0, -1);
                else
                    newRect.adjust(1, 0, -1, 0);
            }
            return newRect;
        }
    } break;
    default:
        break;
    }

    return Super::subControlRect(control, option, subControl, widget);
}

int StudioStyle::styleHint(
        StyleHint hint,
        const QStyleOption *option,
        const QWidget *widget,
        QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_ScrollBar_Transient:
        return true;
    default:
        break;
    }
    return Super::styleHint(hint, option, widget, returnData);
}

int StudioStyle::pixelMetric(
        PixelMetric metric,
        const QStyleOption *option,
        const QWidget *widget) const
{
    switch (metric) {
    case PM_SmallIconSize:
    case PM_LayoutLeftMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutHorizontalSpacing:
    case PM_MenuHMargin:
    case PM_SubMenuOverlap:
    case PM_MenuPanelWidth:
    case PM_MenuBarHMargin:
    case PM_MenuBarVMargin:
    case PM_ToolBarSeparatorExtent:
    case PM_ToolBarFrameWidth:
        if (isQmlEditorMenu(widget)) {
            switch (metric) {
            case PM_SmallIconSize:
                return 10;
            case PM_LayoutLeftMargin:
            case PM_LayoutRightMargin:
                return 7;
            case PM_LayoutHorizontalSpacing:
                return 12;
            case PM_MenuHMargin:
                return 5;
            case PM_SubMenuOverlap:
                return 0;
            case PM_MenuPanelWidth:
            case PM_MenuBarHMargin:
            case PM_MenuBarVMargin:
            case PM_ToolBarSeparatorExtent:
            case PM_ToolBarFrameWidth:
                return 1;
            default:
                return 0;
            }
        }
        break;
    case PM_ButtonShiftVertical:
    case PM_ButtonShiftHorizontal:
    case PM_MenuBarPanelWidth:
    case PM_ToolBarItemMargin:
        return 0;
    case PM_ToolBarItemSpacing:
        return 4;
    case PM_ToolBarExtensionExtent:
        return 29;
    case PM_ScrollBarExtent: {
        if (styleHint(SH_ScrollBar_Transient, option, widget))
            return 10;
        return 14;
    } break;
    case PM_ScrollBarSliderMin:
        return 30;
    case PM_SliderLength:
        return 5;
    case PM_SliderThickness: {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            return (slider->orientation == Qt::Horizontal
                        ? slider->rect.height()
                        : slider->rect.width()) - 1;
        }
    } break;
    case PM_SliderControlThickness:
        return 2;
    default:
        break;
    }
    return Super::pixelMetric(metric, option, widget);
}

QPalette StudioStyle::standardPalette() const
{
    return d->stdPalette;
}

void StudioStyle::polish(QWidget *widget)
{
    if (widget && widget->property("_q_custom_style_skipolish").toBool())
        return;
    Super::polish(widget);
}

void StudioStyle::drawQmlEditorIcon(
        PrimitiveElement element,
        const QStyleOption *option,
        const char *propertyName,
        QPainter *painter,
        const QWidget *widget) const
{
    if (option->styleObject && option->styleObject->property(propertyName).isValid()) {
        const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (mbi) {
            const bool isCheckable = mbi->checkType != QStyleOptionMenuItem::NotCheckable;
            const bool isEnabled = mbi->state & State_Enabled;
            const bool isActive = mbi->state & State_Selected;
            const bool isChecked = isCheckable && mbi->checked;
            QIcon icon = mbi->styleObject->property(propertyName).value<QIcon>();
            QPixmap pix = getPixmapFromIcon(icon, mbi->rect.size(),
                                            isEnabled, isActive, isChecked);
            drawItemPixmap(painter, mbi->rect, Qt::AlignCenter, pix);
            return;
        }
    }
    Super::drawPrimitive(element, option, painter, widget);
}
