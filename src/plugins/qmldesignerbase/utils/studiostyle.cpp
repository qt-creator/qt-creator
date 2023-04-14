// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "studiostyle.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QMenu>
#include <QPainter>
#include <QStyleOption>

using namespace Utils;

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

QPixmap getPixmapFromIcon(const QIcon &icon, const QSize &size, bool enabled, bool active, bool checked)
{
    QIcon::Mode mode = enabled ? ((active) ? QIcon::Active : QIcon::Normal) : QIcon::Disabled;
    QIcon::State state = (checked) ? QIcon::On : QIcon::Off;
    return icon.pixmap(size, mode, state);
}

struct StudioShortcut {
    StudioShortcut(const QStyleOptionMenuItem *option,
                      const QString &shortcutText)
        : shortcutText(shortcutText)
        , enabled(option->state & QStyle::State_Enabled)
        , active(option->state & QStyle::State_Selected)
        , font(option->font)
        , fm(font)
        , defaultHeight(fm.height())
        , spaceConst(fm.boundingRect(".").width())
    {
        reset();

        if (backspaceMatch(shortcutText).hasMatch())
            backspaceIcon = option->styleObject->property("backspaceIcon").value<QIcon>();
    }

    QSize getSize()
    {
        if (isFirstParticle)
            calcResult();
        return _size;
    }

    QPixmap getPixmap()
    {
        if (!isFirstParticle && !_pixmap.isNull())
            return _pixmap;

        _pixmap = QPixmap(getSize());
        _pixmap.fill(Qt::transparent);
        QPainter painter(&_pixmap);
        painter.setFont(font);
        QPen pPen = painter.pen();
        pPen.setColor(studioTextColor(enabled, active, false));
        painter.setPen(pPen);
        calcResult(&painter);
        painter.end();

        return _pixmap;
    }

private:
    void applySize(const QSize &itemSize) {
        width += itemSize.width();
        height = std::max(height, itemSize.height());
        if (isFirstParticle)
            isFirstParticle = false;
        else
            width += spaceConst;
    };

    void addText(const QString &txt, QPainter *painter = nullptr)
    {
        if (txt.size()) {
            int textWidth = fm.boundingRect(txt).width();
            QSize itemSize = {textWidth, defaultHeight};
            if (painter) {
                static const QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);
                QRect placeRect({width, 0}, itemSize);
                painter->drawText(placeRect, txt, textOption);
            }
            applySize(itemSize);
        }
    };

    void addPixmap(const QPixmap &pixmap, QPainter *painter = nullptr)
    {
        if (painter)
            painter->drawPixmap(QRect({width, 0}, pixmap.size()), pixmap);

        applySize(pixmap.size());
    };

    void calcResult(QPainter *painter = nullptr)
    {
        reset();
#ifndef QT_NO_SHORTCUT
        if (!shortcutText.isEmpty()) {
            int fwdIndex = 0;

            QRegularExpressionMatch mMatch = backspaceMatch(shortcutText);
            int matchCount = mMatch.lastCapturedIndex();

            for (int i = 0; i <= matchCount; ++i) {
                QString mStr = mMatch.captured(i);
                QSize iconSize(defaultHeight * 3, defaultHeight);
                const QList<QSize> iconSizes = backspaceIcon.availableSizes();
                if (iconSizes.size())
                    iconSize = iconSizes.last();
                double aspectRatio = (defaultHeight + .0) / iconSize.height();
                int newWidth = iconSize.width() * aspectRatio;

                QPixmap pixmap = getPixmapFromIcon(backspaceIcon,
                                                   {newWidth, defaultHeight},
                                                   enabled, active, false);

                int lIndex = shortcutText.indexOf(mStr, fwdIndex);
                int diffChars = lIndex - fwdIndex;
                addText(shortcutText.mid(fwdIndex, diffChars), painter);
                addPixmap(pixmap, painter);
                fwdIndex = lIndex + mStr.size();
            }
            addText(shortcutText.mid(fwdIndex), painter);
        }
#endif
        _size = {width, height};
    }

    void reset()
    {
        isFirstParticle = true;
        width = 0;
        height = 0;
    }

    inline QRegularExpressionMatch backspaceMatch(const QString &str) const
    {
        static const QRegularExpression backspaceDetect(
                    "\\+*backspace\\+*",
                    QRegularExpression::CaseInsensitiveOption);
        return backspaceDetect.match(str);
    }

    const QString shortcutText;
    const bool enabled;
    const bool active;
    const QFont font;
    const QFontMetrics fm;
    const int defaultHeight;
    const int spaceConst;
    QIcon backspaceIcon;
    bool isFirstParticle = true;

    int width = 0;
    int height = 0;
    QSize _size;
    QPixmap _pixmap;
};

} // blank namespace

class StudioStylePrivate
{
public:
    explicit StudioStylePrivate();

public:
    QPalette stdPalette;
};

StudioStylePrivate::StudioStylePrivate()
{
    auto color = [] (Theme::Color c) {
        return creatorTheme()->color(c);
    };

    {
        stdPalette.setColorGroup(
                    QPalette::Disabled, // group
                    color(Theme::DStextColorDisabled), // windowText
                    color(Theme::DScontrolBackgroundDisabled), // button
                    color(Theme::DScontrolOutlineDisabled), // light
                    color(Theme::DStextSelectedTextColor), // dark
                    color(Theme::DSstatusbarBackground), // mid
                    color(Theme::DStextColorDisabled), // text
                    color(Theme::DStextColorDisabled), // brightText
                    color(Theme::DStoolbarIcon_blocked), // base
                    color(Theme::DStoolbarIcon_blocked) // window
                    );

        stdPalette.setColorGroup(
                    QPalette::Inactive, // group
                    color(Theme::DStextColor), // windowText
                    color(Theme::DScontrolBackground), // button
                    color(Theme::DStoolbarBackground), // light
                    color(Theme::DSstatusbarBackground), // dark
                    color(Theme::DScontrolBackground), // mid
                    color(Theme::DStextColor), // text
                    color(Theme::DStextColor), // brightText
                    color(Theme::DStoolbarBackground), // base
                    color(Theme::DStoolbarBackground) // window
                    );

        stdPalette.setColorGroup(
                    QPalette::Active, // group
                    color(Theme::DStextSelectedTextColor), // windowText
                    color(Theme::DSnavigatorItemBackgroundHover), // button
                    color(Theme::DSstateBackgroundColor_hover), // light
                    color(Theme::DSpanelBackground), // dark
                    color(Theme::DSnavigatorItemBackgroundHover), // mid
                    color(Theme::DStextSelectedTextColor), // text
                    color(Theme::DStextSelectedTextColor), // brightText
                    color(Theme::DStoolbarBackground), // base
                    color(Theme::DStoolbarBackground) // window
                    );
    }
}

StudioStyle::StudioStyle(QStyle *style)
    : QProxyStyle(style)
    , d(new StudioStylePrivate)
{
}

StudioStyle::StudioStyle(const QString &key)
    : QProxyStyle(key)
    , d(new StudioStylePrivate)
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
    case PE_FrameDefaultButton:
    {
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
    }
        break;

    case PE_IndicatorToolBarSeparator:
    {
        bool horizontal = option->state & State_Horizontal;
        int thickness = pixelMetric(PM_ToolBarSeparatorExtent, option, widget);
        QRect colorRect;
        if (horizontal) {
            colorRect = {option->rect.center().x() - thickness / 2  , option->rect.top() + 2,
                         thickness , option->rect.height() - 4};
        } else {
            colorRect = {option->rect.left() + 2, option->rect.center().y() - thickness / 2,
                         option->rect.width() - 4, thickness};
        }

        // The separator color is currently the same as toolbar bg
        painter->fillRect(colorRect,
                          creatorTheme()->color(Theme::DStoolbarBackground));
    }
        break;

    default:
    {
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
                int additionalMargin = forwardX /*hmargin*/;
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

    case CC_Slider:
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

            int lineWidth = pixelMetric(QStyle::PM_DefaultFrameWidth, option, widget);
            Theme::Color themeframeColor = enabled
                    ? interaction
                      ? Theme::DSstateControlBackgroundColor_hover // Pressed
                      : grooveHover
                          ? Theme::DSstateSeparatorColor // GrooveHover
                          : Theme::DSpopupBackground // Idle
                    : Theme::DSpopupBackground; // Disabled

            QColor frameColor = creatorTheme()->color(themeframeColor);

            if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
                Theme::Color bgPlusColor = enabled
                        ? interaction
                          ? Theme::DSstateControlBackgroundColor_hover // Pressed
                          : grooveHover
                              ? Theme::DSstateSeparatorColor // GrooveHover
                              : Theme::DStoolbarBackground // Idle
                        : Theme::DStoolbarBackground; // Disabled
                Theme::Color bgMinusColor = Theme::DSpopupBackground;

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
                painter->setBrush(creatorTheme()->color(bgPlusColor));
                painter->drawRoundedRect(plusRect, borderRadius, borderRadius);
                painter->setBrush(creatorTheme()->color(bgMinusColor));
                painter->drawRoundedRect(minusRect, borderRadius, borderRadius);
                painter->restore();
            }

            if (option->subControls & SC_SliderTickmarks) {
                Theme::Color tickPen = enabled
                        ? activeFocus
                          ? Theme::DSstateBackgroundColor_hover
                          : Theme::DSBackgroundColorAlternate
                        : Theme::DScontrolBackgroundDisabled;

                painter->setPen(tickPen);
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
            if ((option->subControls & SC_SliderHandle) ) {
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
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(frameColor, lineWidth));
                painter->drawRoundedRect(groove, borderRadius, borderRadius);
            }
            painter->restore();
        }
        break;

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
    case CT_MenuItem:
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
        break;

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

    if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
        switch (subControl) {
        case SubControl::SC_SliderGroove:
            return option->rect;
        case SubControl::SC_SliderHandle:
        {
            QRect retval = Super::subControlRect(control, option, subControl, widget);
            int thickness = 2;
            QPoint center = retval.center();
            const QRect &rect = slider->rect;
            if (slider->orientation == Qt::Horizontal)
                return {center.x() - thickness, rect.top(), (thickness * 2) + 1, rect.height()};
            else
                return {rect.left(), center.y() - thickness, rect.width(), (thickness * 2) + 1};
        }
            break;
        default:
            break;
        }
    }
    return Super::subControlRect(control, option, subControl, widget);
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
    default:
        break;
    }
    return Super::pixelMetric(metric, option, widget);
}

QPalette StudioStyle::standardPalette() const
{
    return d->stdPalette;
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
