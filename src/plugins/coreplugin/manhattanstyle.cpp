// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "manhattanstyle.h"

#include "generalsettings.h"

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/styleanimator.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QToolButton>

using namespace Utils;

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

static bool isInUnstyledDialogOrPopup(const QWidget *widget)
{
    // Do not style contents of dialogs or popups without "panelwidget" property
    const QWidget *window = widget->window();
    if (window->property(StyleHelper::C_PANEL_WIDGET).toBool())
        return false;
    const Qt::WindowType windowType = window->windowType();
    return (windowType == Qt::Dialog || windowType == Qt::Popup);
}

// Consider making this a QStyle state
bool panelWidget(const QWidget *widget)
{
    if (!widget)
        return false;

    if (isInUnstyledDialogOrPopup(widget))
        return false;

    if (qobject_cast<const FancyMainWindow *>(widget))
        return true;

    if (qobject_cast<const QTabBar *>(widget))
        return styleEnabled(widget);

    if (qobject_cast<const QScrollArea *>(widget)) // See DebuggerMainWindowPrivate
        return widget->property(StyleHelper::C_PANEL_WIDGET_SINGLE_ROW).toBool();

    const QWidget *p = widget;
    while (p) {
        if (qobject_cast<const QToolBar *>(p) ||
            qobject_cast<const QStatusBar *>(p) ||
            qobject_cast<const QMenuBar *>(p) ||
            p->property(StyleHelper::C_PANEL_WIDGET).toBool())
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

    if (isInUnstyledDialogOrPopup(widget))
        return false;

    const QWidget *p = widget;
    while (p) {
        if (p->property(StyleHelper::C_LIGHT_COLORED).toBool())
            return true;
        p = p->parentWidget();
    }
    return false;
}

static bool isDarkFusionStyle(const QStyle *style)
{
    return creatorTheme()->flag(Theme::DarkUserInterface)
            && strcmp(style->metaObject()->className(), "QFusionStyle") == 0;
}

class ManhattanStylePrivate
{
public:
    explicit ManhattanStylePrivate();

public:
    const QIcon extButtonIcon;
    const QPixmap closeButtonPixmap;
    StyleAnimator animator;
};

ManhattanStylePrivate::ManhattanStylePrivate()
    : extButtonIcon(Utils::Icons::TOOLBAR_EXTENSION.icon())
    , closeButtonPixmap(Utils::Icons::CLOSE_FOREGROUND.pixmap())
{
}

ManhattanStyle::ManhattanStyle(const QString &baseStyleName)
    : QProxyStyle(QStyleFactory::create(baseStyleName))
    , d(new ManhattanStylePrivate())
{
    Core::Internal::GeneralSettings::applyToolbarStyleFromSettings();
}

ManhattanStyle::~ManhattanStyle()
{
    delete d;
    d = nullptr;
}

QPixmap ManhattanStyle::generatedIconPixmap(
        QIcon::Mode iconMode,
        const QPixmap &pixmap,
        const QStyleOption *opt) const
{
    return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

QSize ManhattanStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                       const QSize &size, const QWidget *widget) const
{
    QSize newSize = QProxyStyle::sizeFromContents(type, option, size, widget);

    switch (type) {
    case CT_Splitter:
        if (widget && widget->property(StyleHelper::C_MINI_SPLITTER).toBool())
            newSize = QSize(1, 1);
        break;
    case CT_ComboBox:
        if (panelWidget(widget))
            newSize += QSize(14, 0);
        break;
    default:
        break;
    }
    return newSize;
}

QRect ManhattanStyle::subElementRect(
        SubElement element,
        const QStyleOption *option,
        const QWidget *widget) const
{
    return QProxyStyle::subElementRect(element, option, widget);
}

QRect ManhattanStyle::subControlRect(
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
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

QStyle::SubControl ManhattanStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                         const QPoint &pos, const QWidget *widget) const
{
    return QProxyStyle::hitTestComplexControl(control, option, pos, widget);
}

int ManhattanStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int retval = QProxyStyle::pixelMetric(metric, option, widget);

    switch (metric) {
#ifdef Q_OS_MACOS
    case PM_MenuButtonIndicator:
        if (widget && option && option->type == QStyleOption::SO_ToolButton)
            return 12;
        break;
#endif
    case PM_SplitterWidth:
        if (widget && widget->property(StyleHelper::C_MINI_SPLITTER).toBool())
            retval = 1;
        break;
    case PM_ToolBarIconSize:
    case PM_ButtonIconSize:
        if (panelWidget(widget))
            retval = 16;
        break;
    case PM_SmallIconSize:
        retval = 16;
        break;
    case PM_DockWidgetHandleExtent:
    case PM_DockWidgetSeparatorExtent:
        retval = 1;
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
    return QProxyStyle::standardPalette();
}

void ManhattanStyle::polish(QApplication *app)
{
    QProxyStyle::polish(app);
}

void ManhattanStyle::unpolish(QApplication *app)
{
    QProxyStyle::unpolish(app);
}

QPalette panelPalette(const QPalette &oldPalette, bool lightColored = false)
{
    QColor color = creatorTheme()->color(lightColored ? Theme::PanelTextColorDark
                                                      : Theme::PanelTextColorLight);
    QPalette pal = oldPalette;
    pal.setBrush(QPalette::All, QPalette::WindowText, color);
    pal.setBrush(QPalette::All, QPalette::ButtonText, color);
    if (lightColored)
        color.setAlpha(100);
    else
        color = creatorTheme()->color(Theme::IconsDisabledColor);
    pal.setBrush(QPalette::Disabled, QPalette::WindowText, color);
    pal.setBrush(QPalette::Disabled, QPalette::ButtonText, color);
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
        // So that text isn't cutoff in line-edits, comboboxes... etc.
        const int height = qMax(StyleHelper::navigationWidgetHeight(), QApplication::fontMetrics().height());
        if (qobject_cast<QToolButton*>(widget)) {
            widget->setMinimumWidth(
                StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleCompact ? 24 : 28);
            widget->setAttribute(Qt::WA_Hover);
            widget->setMaximumHeight(height - 2);
        } else if (qobject_cast<QLineEdit*>(widget)) {
            widget->setAttribute(Qt::WA_Hover);
            widget->setFixedHeight(height - (StyleHelper::toolbarStyle()
                                                     == StyleHelper::ToolbarStyleCompact ? 1 : 3));
        } else if (qobject_cast<QLabel*>(widget) || qobject_cast<QSpinBox*>(widget)
                   || qobject_cast<QCheckBox*>(widget)) {
            widget->setPalette(panelPalette(widget->palette(), lightColored(widget)));
        } else if ((qobject_cast<QToolBar*>(widget) && !StyleHelper::isQDSTheme())
                   || widget->property(StyleHelper::C_PANEL_WIDGET_SINGLE_ROW).toBool()) {
            widget->setFixedHeight(height);
        } else if (qobject_cast<QStatusBar*>(widget)) {
            const bool flatAndNotCompact =
                StyleHelper::toolbarStyle() != StyleHelper::ToolbarStyleCompact
                                           && creatorTheme()->flag(Theme::FlatToolBars);
            widget->setFixedHeight(height + (flatAndNotCompact ? 3 : 2));
        } else if (qobject_cast<QComboBox*>(widget)) {
            const bool isLightColored = lightColored(widget);
            QPalette palette = panelPalette(widget->palette(), isLightColored);
            if (!isLightColored)
                palette.setBrush(QPalette::All, QPalette::WindowText,
                                 creatorTheme()->color(Theme::ComboBoxTextColor));
            widget->setPalette(palette);
            widget->setMaximumHeight(height - 2);
            widget->setAttribute(Qt::WA_Hover);
        } else if (qobject_cast<QScrollArea*>(widget)
                   && widget->property(StyleHelper::C_PANEL_WIDGET_SINGLE_ROW).toBool()) {
            widget->setFixedHeight(height);
        }
    }
}

void ManhattanStyle::unpolish(QWidget *widget)
{
    QProxyStyle::unpolish(widget);
    if (panelWidget(widget)) {
        widget->setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
        if (qobject_cast<QTabBar*>(widget)
                || qobject_cast<QToolBar*>(widget)
                || qobject_cast<QComboBox*>(widget)) {
            widget->setAttribute(Qt::WA_Hover, false);
        }
    }
}

void ManhattanStyle::polish(QPalette &pal)
{
    QProxyStyle::polish(pal);
}

QPixmap ManhattanStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                                       const QWidget *widget) const
{
    if (widget && !panelWidget(widget))
        return QProxyStyle::standardPixmap(standardPixmap, opt, widget);

    QPixmap pixmap;
    switch (standardPixmap) {
    case QStyle::SP_TitleBarCloseButton:
        pixmap = d->closeButtonPixmap;
        break;
    default:
        pixmap = QProxyStyle::standardPixmap(standardPixmap, opt, widget);
        break;
    }
    return pixmap;
}

QIcon ManhattanStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    QIcon icon;
    switch (standardIcon) {
    case QStyle::SP_ToolBarHorizontalExtensionButton:
        icon = d->extButtonIcon;
        break;
    case QStyle::SP_ComputerIcon:
    {
        icon = QProxyStyle::standardIcon(standardIcon, option, widget);

        // Ubuntu has in some versions a 16x16 icon, see QTCREATORBUG-12832
        const QList<QSize> &sizes = icon.availableSizes();
        if (Utils::allOf(sizes, [](const QSize &size) { return size.width() < 32;}))
            icon = QIcon(":/utils/images/Desktop.png");
    }
        break;
    default:
        icon = QProxyStyle::standardIcon(standardIcon, option, widget);
        break;
    }
    return icon;
}

int ManhattanStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                              QStyleHintReturn *returnData) const
{
    int ret = QProxyStyle::styleHint(hint, option, widget, returnData);
    switch (hint) {
    case QStyle::SH_EtchDisabledText:
        if (panelWidget(widget) || qobject_cast<const QMenu *> (widget) )
            ret = false;
        break;
    case QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren:
        ret = true;
        break;
    case QStyle::SH_ItemView_ActivateItemOnSingleClick:
        // default depends on the style
        if (widget) {
            QVariant activationMode = widget->property(activationModeC);
            if (activationMode.isValid())
                ret = activationMode.toBool();
        }
        break;
    case QStyle::SH_FormLayoutFieldGrowthPolicy:
        // The default in QMacStyle, FieldsStayAtSizeHint, is just always the wrong thing
        // Use the same as on all other shipped styles
        if (Utils::HostOsInfo::isMacHost())
            ret = QFormLayout::AllNonFixedFieldsGrow;
        break;
    case QStyle::SH_Widget_Animation_Duration:
        if (widget && widget->inherits("QTreeView"))
            ret = 0;
        break;
    case QStyle::SH_ComboBox_AllowWheelScrolling:
        // Turn this on only when simultaneously pressing Ctrl, to prevent accidental current
        // index change, e.g. on a scroll view
        ret = QGuiApplication::keyboardModifiers()
              == (HostOsInfo::isMacHost() ? Qt::MetaModifier : Qt::ControlModifier);
        break;
    case QStyle::SH_Slider_AbsoluteSetButtons:
        // Make QSlider jump on left mouse click
        ret = Qt::LeftButton | Qt::MiddleButton | Qt::RightButton;
        break;
    default:
        break;
    }
    return ret;
}

static void drawPrimitiveTweakedForDarkTheme(QStyle::PrimitiveElement element,
                                             const QStyleOption *option,
                                             QPainter *painter, const QWidget *widget)
{
    const bool hasFocus = option->state & QStyle::State_HasFocus;
    const bool isChecked = option->state & QStyle::State_On;
    const bool isPartiallyChecked = option->state & QStyle::State_NoChange;
    const bool isEnabled = option->state & QStyle::State_Enabled;
    const bool isSunken = option->state & QStyle::State_Sunken;

    const QColor frameColor = isEnabled ? option->palette.color(QPalette::Mid).darker(132)
                                        : creatorTheme()->color(Theme::BackgroundColorDisabled);
    const QColor indicatorColor = isEnabled ? option->palette.color(QPalette::Mid).darker(90)
                                            : creatorTheme()->color(Theme::BackgroundColorDisabled);
    const QColor bgColor = isSunken ? option->palette.color(QPalette::Mid).darker()
                                    : option->palette.color(QPalette::Window);
    const QColor hlColor = option->palette.color(QPalette::Highlight);

    QPen framePen(hasFocus ? hlColor : frameColor, 1);
    framePen.setJoinStyle(Qt::MiterJoin);
    QPen indicatorPen(indicatorColor, 1);
    indicatorPen.setJoinStyle(Qt::MiterJoin);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    switch (element) {
    case QStyle::PE_Frame: {
        const QRectF frameRectF = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        painter->setPen(framePen);
        painter->drawRect(frameRectF);
        break;
    }
    case QStyle::PE_FrameLineEdit: {
        const bool isComboBox = widget && widget->inherits("QComboBox");
        const QRectF frameRectF =
                QRectF(option->rect).adjusted(0.5, 0.5, isComboBox ? -8.5 : -0.5, -0.5);
        painter->setPen(framePen);
        painter->drawRect(frameRectF);
        break;
    }
    case QStyle::PE_FrameGroupBox: {
        // Snippet from QFusionStyle::drawPrimitive - BEGIN
        static const int groupBoxTopMargin =  3;
        int topMargin = 0;
        auto control = dynamic_cast<const QGroupBox *>(widget);
        if (control && !control->isCheckable() && control->title().isEmpty()) {
            // Shrinking the topMargin if Not checkable AND title is empty
            topMargin = groupBoxTopMargin;
        } else {
            const int exclusiveIndicatorHeight = widget ? widget->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight) : 0;
            topMargin = qMax(exclusiveIndicatorHeight,
                             option->fontMetrics.height()) + groupBoxTopMargin;
        }
        // Snippet from QFusionStyle::drawPrimitive - END

        const QRectF frameRectF = QRectF(option->rect).adjusted(0.5, topMargin + 0.5, -0.5, -0.5);
        painter->setPen(framePen);
        if (isEnabled)
            painter->setOpacity(0.5);
        painter->drawRect(frameRectF);
        break;
    }
    case QStyle::PE_IndicatorRadioButton: {
        const double lineWidth = 1.666;
        const double o = lineWidth / 2;
        indicatorPen.setWidth(lineWidth);
        painter->setPen(framePen);
        if (isEnabled)
            painter->setBrush(bgColor);
        painter->drawRoundedRect(QRectF(option->rect).adjusted(o, o, -o, -o),
                                 100, 100, Qt::RelativeSize);

        if (isChecked) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(indicatorColor);
            const double o = 4.25;
            painter->drawRoundedRect(QRectF(option->rect).adjusted(o, o, -o, -o),
                                     100, 100, Qt::RelativeSize);
        }
        break;
    }
    case QStyle::PE_IndicatorCheckBox: {
        const QRectF frameRectF = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        painter->setPen(framePen);
        if (isEnabled)
            painter->setBrush(bgColor);
        painter->drawRect(frameRectF);

        if (isPartiallyChecked) {
            QPen outline(indicatorColor, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter->setPen(outline);
            QColor fill(frameColor);
            fill.setAlphaF(0.8f);
            painter->setBrush(fill);
            const double o = 3.5;
            painter->drawRect(QRectF(option->rect).adjusted(o, o, -o, -o));
        } else if (isChecked) {
            const double o = 3;
            const QRectF r = QRectF(option->rect).adjusted(o, o, -o, -o);
            QPen checkMarkPen(indicatorColor, 1.75, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter->setPen(checkMarkPen);
            painter->drawPolyline(QPolygonF({QPointF(r.left(), r.top() + r.height() / 2),
                                             QPointF(r.left() + r.width() / 2.3, r.bottom()),
                                             r.topRight()}));
        }
        break;
    }
    case QStyle::PE_IndicatorTabClose: {
        QWindow *window = widget ? widget->window()->windowHandle() : nullptr;
        QRect iconRect = QRect(0, 0, 16, 16);
        iconRect.moveCenter(option->rect.center());
        const QIcon::Mode mode = !isEnabled ? QIcon::Disabled : QIcon::Normal;
        const static QIcon closeIcon = Utils::Icons::CLOSE_FOREGROUND.icon();
        if (option->state & QStyle::State_MouseOver && widget)
            widget->style()->drawPrimitive(QStyle::PE_PanelButtonCommand, option, painter, widget);
        const int devicePixelRatio = widget ? widget->devicePixelRatio() : 1;
        const QPixmap iconPx =
                closeIcon.pixmap(window, iconRect.size() * devicePixelRatio, mode);
        painter->drawPixmap(iconRect, iconPx);
        break;
    }
    default:
        QTC_ASSERT_STRING("Unhandled QStyle::PrimitiveElement case");
        break;
    }
    painter->restore();
}

void ManhattanStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const
{
    if (panelWidget(widget)) {
        drawPrimitiveForPanelWidget(element, option, painter, widget);
    } else {
        const bool tweakDarkTheme =
                (element == PE_Frame
                 || element == PE_FrameLineEdit
                 || element == PE_FrameGroupBox
                 || element == PE_IndicatorRadioButton
                 || element == PE_IndicatorCheckBox
                 || element == PE_IndicatorTabClose)
                && isDarkFusionStyle(baseStyle());
        if (tweakDarkTheme)
            drawPrimitiveTweakedForDarkTheme(element, option, painter, widget);
        else
            QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
}

void ManhattanStyle::drawPrimitiveForPanelWidget(PrimitiveElement element,
                                                 const QStyleOption *option,
                                                 QPainter *painter,
                                                 const QWidget *widget) const
{
    int state = option->state;
    QRect rect = option->rect;
    switch (element) {
    case PE_IndicatorDockWidgetResizeHandle:
        painter->fillRect(option->rect, creatorTheme()->color(Theme::DockWidgetResizeHandleColor));
        break;
    case PE_FrameDockWidget:
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    case PE_PanelLineEdit:
        {
            painter->save();

            // Fill the line edit background
            QRectF backgroundRect = option->rect;
            const bool enabled = option->state & State_Enabled;
            if (Utils::creatorTheme()->flag(Theme::FlatToolBars)) {
                painter->save();
                if (!enabled)
                    painter->setOpacity(0.75);
                QBrush baseBrush = option->palette.base();
                if (widget && qobject_cast<const QSpinBox *>(widget->parentWidget()))
                    baseBrush = creatorTheme()->color(Theme::DScontrolBackgroundDisabled);
                painter->fillRect(backgroundRect, baseBrush);
                painter->restore();
            } else {
                backgroundRect.adjust(1, 1, -1, -1);
                painter->setBrushOrigin(backgroundRect.topLeft());
                painter->fillRect(backgroundRect, option->palette.base());

                static const QImage bg(StyleHelper::dpiSpecificImageFile(
                                           QLatin1String(":/utils/images/inputfield.png")));
                static const QImage bg_disabled(StyleHelper::dpiSpecificImageFile(
                                                    QLatin1String(":/utils/images/inputfield_disabled.png")));

                StyleHelper::drawCornerImage(enabled ? bg : bg_disabled,
                                             painter, option->rect, 5, 5, 5, 5);
            }

            const bool hasFocus = state & State_HasFocus;
            if (enabled && (hasFocus || state & State_MouseOver)) {
                QColor hover = StyleHelper::baseColor();
                hover.setAlpha(hasFocus ? 100 : 50);
                painter->setPen(QPen(hover, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
                painter->drawRect(backgroundRect.adjusted(0.5, 0.5, -0.5, -0.5));
            }
            painter->restore();
        }
        break;

    case PE_FrameStatusBarItem:
        break;

    case PE_PanelButtonTool: {
        painter->save();
        const bool animating = (state & State_Animating);

        if (widget && !animating) {
                auto w = const_cast<QWidget *> (widget);
                int oldState = w->property("_q_stylestate").toInt();
                QRect oldRect = w->property("_q_stylerect").toRect();
                QRect newRect = w->rect();
                w->setProperty("_q_stylestate", (int)option->state);
                w->setProperty("_q_stylerect", w->rect());

                // Determine the animated transition
                bool doTransition = ((state & State_On) != (oldState & State_On) ||
                                     (state & State_MouseOver)  != (oldState & State_MouseOver));
                if (oldRect != newRect) {
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
                    auto t = new Transition;
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

        Animation *anim = d->animator.widgetAnimation(widget);

        if (!animating && anim) {
                anim->paint(painter, option);
        } else {
                const bool pressed = option->state & State_Sunken || option->state & State_On
                                     || (widget && widget->property(StyleHelper::C_HIGHLIGHT_WIDGET).toBool());
                painter->setPen(StyleHelper::sidebarShadow());
                if (pressed) {
                    StyleHelper::drawPanelBgRect(
                        painter, rect, creatorTheme()->color(Theme::FancyToolButtonSelectedColor));
                    if (StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleCompact
                        && !creatorTheme()->flag(Theme::FlatToolBars)) {
                        const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
                        painter->drawLine(borderRect.topLeft() + QPointF(1, 0), borderRect.topRight() - QPointF(1, 0));
                        painter->drawLine(borderRect.topLeft(), borderRect.bottomLeft());
                        painter->drawLine(borderRect.topRight(), borderRect.bottomRight());
                    }
                } else if (option->state & State_Enabled && option->state & State_MouseOver) {
                    if (widget->property(StyleHelper::C_TOOLBAR_ACTIONWIDGET).toBool()) {
                        painter->save();
                        painter->setBrush(creatorTheme()->color(Theme::FancyToolButtonHoverColor));
                        painter->drawRoundedRect(rect, 5, 5);
                        painter->restore();
                    } else {
                        StyleHelper::drawPanelBgRect(painter,
                                                     rect,
                                                     creatorTheme()->color(
                                                         Theme::FancyToolButtonHoverColor));
                    }
                }
            if (option->state & State_HasFocus && (option->state & State_KeyboardFocusChange)) {
                QColor highlight = option->palette.highlight().color();
                highlight.setAlphaF(0.4f);
                painter->setPen(QPen(highlight.lighter(), 1));
                highlight.setAlphaF(0.3f);
                painter->setBrush(highlight);
                painter->setRenderHint(QPainter::Antialiasing);
                const QRectF rect = option->rect;
                painter->drawRoundedRect(rect.adjusted(2.5, 2.5, -2.5, -2.5), 2, 2);
            }
        }
        painter->restore();
    }
        break;

    case PE_PanelStatusBar:
        {
            const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
            painter->save();
            if (creatorTheme()->flag(Theme::FlatToolBars)) {
                painter->fillRect(rect, StyleHelper::baseColor());
            } else {
                QLinearGradient grad = StyleHelper::statusBarGradient(rect);
                painter->fillRect(rect, grad);
                painter->setPen(QColor(255, 255, 255, 60));
                painter->drawLine(borderRect.topLeft() + QPointF(0, 1),
                                  borderRect.topRight()+ QPointF(0, 1));
                painter->setPen(StyleHelper::borderColor().darker(110)); //TODO: make themable
                painter->drawLine(borderRect.topLeft(), borderRect.topRight());
            }
            if (creatorTheme()->flag(Theme::DrawToolBarBorders)) {
                painter->setPen(StyleHelper::toolBarBorderColor());
                painter->drawLine(borderRect.topLeft(), borderRect.topRight());
            }
            painter->restore();
        }
        break;

    case PE_IndicatorToolBarSeparator:
        if (!StyleHelper::isQDSTheme()) {
            QRect separatorRect = rect;
            separatorRect.setLeft(rect.width() / 2);
            separatorRect.setWidth(1);
            drawButtonSeparator(painter, separatorRect, false);
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
            QColor dark = StyleHelper::borderColor();
            dark.setAlphaF(0.4f);

            QColor light = StyleHelper::baseColor();
            light.setAlphaF(0.4f);

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
        if (qobject_cast<const QMenu *>(widget)) // leave submenu arrow painting alone
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        else
            StyleHelper::drawArrow(element, painter, option);
        break;

    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

void ManhattanStyle::drawControl(
        ControlElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget) const
{
    if (!panelWidget(widget) && !qobject_cast<const QMenu *>(widget)) {
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    switch (element) {
    case CE_MenuItem:
        painter->save();
        if (const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const bool enabled = mbi->state & State_Enabled;
            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect;
            const QColor color = creatorTheme()->color(enabled
                                                       ? Theme::MenuItemTextColorNormal
                                                       : Theme::MenuItemTextColorDisabled);
            if (color.isValid()) {
                QPalette pal = mbi->palette;
                pal.setBrush(QPalette::Text, color);
                item.palette = pal;
            }

            QProxyStyle::drawControl(element, &item, painter, widget);
        }
        painter->restore();
        break;

    case CE_MenuBarItem:
        painter->save();
        if (const auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const bool act = mbi->state & (State_Sunken | State_Selected);
            const bool dis = !(mbi->state & State_Enabled);

            if (creatorTheme()->flag(Theme::FlatMenuBar))
                painter->fillRect(option->rect, StyleHelper::baseColor());
            else
                StyleHelper::menuGradient(painter, option->rect, option->rect);

            QStyleOptionMenuItem item = *mbi;
            item.rect = mbi->rect;
            QPalette pal = mbi->palette;
            pal.setBrush(QPalette::ButtonText, dis
                ? creatorTheme()->color(Theme::MenuBarItemTextColorDisabled)
                : creatorTheme()->color(Theme::MenuBarItemTextColorNormal));
            item.palette = pal;
            QCommonStyle::drawControl(element, &item, painter, widget);

            if (act) {
                // Fill|
                const QColor fillColor = StyleHelper::alphaBlendedColors(
                            StyleHelper::baseColor(), creatorTheme()->color(Theme::FancyToolButtonHoverColor));
                painter->fillRect(option->rect, fillColor);

                QPalette pal = mbi->palette;
                uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment |= Qt::TextHideMnemonic;
                pal.setBrush(QPalette::Text, creatorTheme()->color(dis
                                                                   ? Theme::IconsDisabledColor
                                                                   : Theme::PanelTextColorLight));
                drawItemText(painter, item.rect, alignment, pal, !dis, mbi->text, QPalette::Text);
            }
        }
        painter->restore();
        break;

    case CE_ComboBoxLabel:
        if (const auto cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (panelWidget(widget)) {
                painter->save();
                QRect editRect = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
                QPalette customPal = cb->palette;
                const bool drawIcon =
                    !(widget && widget->property(StyleHelper::C_HIDE_ICON).toBool());

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

                Qt::TextElideMode elideMode = Qt::ElideRight;
                if (widget && widget->dynamicPropertyNames().contains(StyleHelper::C_ELIDE_MODE))
                    elideMode = widget->property(StyleHelper::C_ELIDE_MODE)
                                    .value<Qt::TextElideMode>();

                QLatin1Char asterisk('*');
                int elideWidth = editRect.width();

                bool notElideAsterisk = elideMode == Qt::ElideRight && widget
                                        && widget->property(StyleHelper::C_NOT_ELIDE_ASTERISK).toBool()
                                        && cb->currentText.endsWith(asterisk)
                                        && option->fontMetrics.horizontalAdvance(cb->currentText)
                                               > elideWidth;

                QString text;
                if (notElideAsterisk) {
                    elideWidth -= option->fontMetrics.horizontalAdvance(asterisk);
                    text = asterisk;
                }
                text.prepend(option->fontMetrics.elidedText(cb->currentText, elideMode, elideWidth));

                if (creatorTheme()->flag(Theme::ComboBoxDrawTextShadow)
                    && (option->state & State_Enabled))
                {
                    painter->setPen(StyleHelper::toolBarDropShadowColor());
                    painter->drawText(editRect.adjusted(1, 0, -1, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
                }
                painter->setPen((option->state & State_Enabled)
                                  ? option->palette.color(QPalette::WindowText)
                                  : creatorTheme()->color(Theme::IconsDisabledColor));
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
            dark.setAlphaF(0.1f);
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
            if (creatorTheme()->flag(Theme::FlatMenuBar))
                painter->fillRect(option->rect, StyleHelper::baseColor());
            else
                StyleHelper::menuGradient(painter, option->rect, option->rect);

            painter->save();
            painter->setPen(StyleHelper::toolBarBorderColor());
            painter->drawLine(option->rect.bottomLeft() + QPointF(0.5, 0.5),
                              option->rect.bottomRight() + QPointF(0.5, 0.5));
            painter->restore();
        }
        break;

    case CE_ToolBar:
        {
            QRect rect = option->rect;
            const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
            bool horizontal = option->state & State_Horizontal;

            // Map offset for global window gradient
            QRect gradientSpan;
            if (widget) {
                QPoint offset = widget->window()->mapToGlobal(option->rect.topLeft()) -
                                widget->mapToGlobal(option->rect.topLeft());
                gradientSpan = QRect(offset, widget->window()->size());
            }

            bool drawLightColored = lightColored(widget);
            // draws the background of the 'Type hierarchy', 'Projects' headers
            if (creatorTheme()->flag(Theme::FlatToolBars))
                painter->fillRect(rect, StyleHelper::toolbarBaseColor(drawLightColored));
            else if (horizontal)
                StyleHelper::horizontalGradient(painter, gradientSpan, rect, drawLightColored);
            else
                StyleHelper::verticalGradient(painter, gradientSpan, rect, drawLightColored);

            if (creatorTheme()->flag(Theme::DrawToolBarHighlights)) {
                if (!drawLightColored)
                    painter->setPen(StyleHelper::toolBarBorderColor());
                else
                    painter->setPen(QColor(0x888888));

                if (horizontal) {
                    // Note: This is a hack to determine if the
                    // toolbar should draw the top or bottom outline
                    // (needed for the find toolbar for instance)
                    const QColor hightLight = creatorTheme()->flag(Theme::FlatToolBars)
                            ? creatorTheme()->color(Theme::FancyToolBarSeparatorColor)
                            : StyleHelper::sidebarHighlight();
                    const QColor borderColor = drawLightColored
                            ? QColor(255, 255, 255, 180) : hightLight;
                    if (widget && widget->property(StyleHelper::C_TOP_BORDER).toBool()) {
                        painter->drawLine(borderRect.topLeft(), borderRect.topRight());
                        painter->setPen(borderColor);
                        painter->drawLine(borderRect.topLeft() + QPointF(0, 1), borderRect.topRight() + QPointF(0, 1));
                    } else {
                        painter->drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
                        painter->setPen(borderColor);
                        painter->drawLine(borderRect.topLeft(), borderRect.topRight());
                    }
                } else {
                    painter->drawLine(borderRect.topLeft(), borderRect.bottomLeft());
                    painter->drawLine(borderRect.topRight(), borderRect.bottomRight());
                }
            }
            if (creatorTheme()->flag(Theme::DrawToolBarBorders)) {
                painter->setPen(StyleHelper::toolBarBorderColor());
                if (widget && widget->property(StyleHelper::C_TOP_BORDER).toBool())
                    painter->drawLine(borderRect.topLeft(), borderRect.topRight());
                else
                    painter->drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
            }
        }
        break;
    case CE_ToolButtonLabel:
        // Directly use QCommonStyle to circumvent funny painting in QMacStyle
        // which ignores the palette and adds an alpha
        QCommonStyle::drawControl(element, option, painter, widget);
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
         return QProxyStyle::drawComplexControl(control, option, painter, widget);

    QRect rect = option->rect;
    switch (control) {
    case CC_ToolButton:
        if (const auto toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            bool reverse = option->direction == Qt::RightToLeft;
            const bool drawborder =
                (widget && widget->property(StyleHelper::C_SHOW_BORDER).toBool());

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
            if (widget && widget->property(StyleHelper::C_HIGHLIGHT_WIDGET).toBool()) {
                label.palette.setColor(QPalette::ButtonText,
                                       creatorTheme()->color(Theme::IconsWarningToolBarColor));
            }
            int fw = pixelMetric(PM_DefaultFrameWidth, option, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);

            drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.state = mflags;
                tool.rect = menuarea.adjusted(1, 1, -1, -1);
                if (mflags & (State_Sunken | State_On | State_Raised)) {
                    painter->setPen(Qt::gray);
                    const QRectF lineRect = QRectF(tool.rect).adjusted(-0.5, 2.5, 0, -2.5);
                    painter->drawLine(lineRect.topLeft(), lineRect.bottomLeft());
                    if (mflags & (State_Sunken)) {
                        QColor shade(0, 0, 0, 50);
                        painter->fillRect(tool.rect.adjusted(0, -1, 1, 1), shade);
                    } else if (!HostOsInfo::isMacHost() && (mflags & State_MouseOver)) {
                        QColor shade(255, 255, 255, 50);
                        painter->fillRect(tool.rect.adjusted(0, -1, 1, 1), shade);
                    }
                }
                tool.rect = tool.rect.adjusted(2, 2, -2, -2);
                drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu
                       && widget && !widget->property(StyleHelper::C_NO_ARROW).toBool()) {
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
        if (const auto cb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            painter->save();
            bool isEmpty = cb->currentText.isEmpty() && cb->currentIcon.isNull();
            bool reverse = option->direction == Qt::RightToLeft;
            const bool drawborder =
                !(widget && widget->property(StyleHelper::C_HIDE_BORDER).toBool());
            const bool drawleftborder =
                (widget && widget->property(StyleHelper::C_DRAW_LEFT_BORDER).toBool());
            const bool alignarrow =
                !(widget && widget->property(StyleHelper::C_ALIGN_ARROW).toBool());

            if (drawborder) {
                drawButtonSeparator(painter, rect, reverse);
                if (drawleftborder)
                    drawButtonSeparator(painter, rect.adjusted(0, 0, -rect.width() + 2, 0), reverse);
            }

            QStyleOption toolbutton = *option;
            if (isEmpty)
                toolbutton.state &= ~(State_Enabled | State_Sunken);
            painter->save();
            if (drawborder) {
                int leftClipAdjust = 0;
                if (drawleftborder)
                    leftClipAdjust = 2;
                painter->setClipRect(toolbutton.rect.adjusted(leftClipAdjust, 0, -2, 0));
            }
            drawPrimitive(PE_PanelButtonTool, &toolbutton, painter, widget);
            painter->restore();
            // Draw arrow
            int menuButtonWidth = 12;
            int left = !reverse ? rect.right() - menuButtonWidth : rect.left();
            int right = !reverse ? rect.right() : rect.left() + menuButtonWidth;
            QRect arrowRect((left + right) / 2 + (reverse ? 6 : -6), rect.center().y() - 3, 9, 9);

            if (!alignarrow) {
                int labelwidth = option->fontMetrics.horizontalAdvance(cb->currentText);
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

void ManhattanStyle::drawButtonSeparator(QPainter *painter, const QRect &rect, bool reverse)
{
    const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        const int margin = 3;
        painter->setPen(creatorTheme()->color(Theme::FancyToolBarSeparatorColor));
        painter->drawLine(borderRect.topRight() + QPointF(0, margin),
                          borderRect.bottomRight() - QPointF(0, margin));
    } else {
        QLinearGradient grad(rect.topRight(), rect.bottomRight());
        grad.setColorAt(0, QColor(255, 255, 255, 20));
        grad.setColorAt(0.4, QColor(255, 255, 255, 60));
        grad.setColorAt(0.7, QColor(255, 255, 255, 50));
        grad.setColorAt(1, QColor(255, 255, 255, 40));
        painter->setPen(QPen(grad, 1));
        painter->drawLine(borderRect.topRight(), borderRect.bottomRight());
        grad.setColorAt(0, QColor(0, 0, 0, 30));
        grad.setColorAt(0.4, QColor(0, 0, 0, 70));
        grad.setColorAt(0.7, QColor(0, 0, 0, 70));
        grad.setColorAt(1, QColor(0, 0, 0, 40));
        painter->setPen(QPen(grad, 1));
        if (!reverse)
           painter->drawLine(borderRect.topRight() - QPointF(1, 0), borderRect.bottomRight() - QPointF(1, 0));
        else
           painter->drawLine(borderRect.topLeft(), borderRect.bottomLeft());
    }
 }
