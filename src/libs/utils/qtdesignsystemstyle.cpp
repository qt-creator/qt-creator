// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtdesignsystemstyle.h"

#include "stylehelper.h"
#include "utilsicons.h"

#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QWidget>
#include <QStyleOption>
#include <QTabBar>

namespace Utils {

using namespace StyleHelper;
using namespace StyleHelper::SpacingTokens;

constexpr TextFormat TabBarTf
    {Theme::Token_Text_Muted, StyleHelper::UiElementH5};
constexpr TextFormat TabBarTfActive
    {Theme::Token_Text_Default, TabBarTf.uiElement};
constexpr TextFormat TabBarTfDisabled
    {Theme::Token_Text_Subtle, TabBarTf.uiElement};

QtDesignSystemStyle::QtDesignSystemStyle()
{
}

QtDesignSystemStyle::~QtDesignSystemStyle() = default;

void QtDesignSystemStyle::drawControl(ControlElement element, const QStyleOption *opt,
                                      QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_TabBarTabLabel: {
        auto tabOpt = qstyleoption_cast<const QStyleOptionTab*>(opt);
        QStyleOptionTab myTabOpt = *tabOpt;
        const QColor c = ((myTabOpt.state & QStyle::State_Enabled) ?
                              (myTabOpt.state & QStyle::State_Selected) ? TabBarTfActive
                                                                        : TabBarTf
                                                                   : TabBarTfDisabled).color();
        myTabOpt.palette.setColor(widget->foregroundRole(), c);
        myTabOpt.state = myTabOpt.state & ~QStyle::State_HasFocus; // Avoid extra focus rect
        QCommonStyle::drawControl(element, &myTabOpt, painter, widget);
        break;
    }
    case CE_TabBarTabShape: {
        const bool selected = opt->state & QStyle::State_Selected;
        const bool enabled = opt->state & QStyle::State_Enabled;
        const bool hovered = !selected && opt->rect.contains(widget->mapFromGlobal(QCursor::pos()));
        auto tabOpt = qstyleoption_cast<const QStyleOptionTab*>(opt);
        const int paddingL = tabOpt->position == QStyleOptionTab::Beginning ? 0 : PaddingHXxs;
        const int paddingR = tabOpt->position == QStyleOptionTab::End ? 0 : PaddingHXxs;
        const QRect shapeR = opt->rect.adjusted(paddingL, 0, -paddingR, 0);
        if (selected || (hovered && enabled)) {
            if (hovered || tabOpt->position == QStyleOptionTab::Moving) {
                StyleHelper::drawCardBg(painter, shapeR.adjusted(0, 0, 0, +SpacingTokens::RadiusS),
                                        creatorColor(Theme::Token_Foreground_Subtle));
            }
            QRect highLightRect = shapeR;
            highLightRect.moveTop(highLightRect.height() - StyleHelper::HighlightThickness);
            const QColor color = creatorColor(enabled ? hovered ? Theme::Token_Text_Subtle
                                                                : Theme::Token_Accent_Default
                                                      : Theme::Token_Foreground_Subtle);
            painter->fillRect(highLightRect, color);
        }
        break;
    }
    default:
        QCommonStyle::drawControl(element, opt, painter, widget);
        break;
    }
}

void QtDesignSystemStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *opt,
                                        QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_FrameTabBarBase: {
        QRectF borderR = opt->rect.toRectF();
        const int lineWidth = 1;
        borderR.setTop(borderR.bottom() - lineWidth);
        const bool enabled = opt->state & QStyle::State_Enabled;
        const QColor color = creatorColor(enabled ? Theme::Token_Stroke_Subtle
                                                  : Theme::Token_Foreground_Subtle);
        painter->fillRect(borderR, color);
        break;
    }
    default:
        QCommonStyle::drawPrimitive(element, opt, painter, widget);
        break;
    }
}

int QtDesignSystemStyle::pixelMetric(PixelMetric m, const QStyleOption *opt,
                                     const QWidget *widget) const
{
    switch (m) {
    case PM_TabBarTabShiftVertical:
    case PM_TabBarTabShiftHorizontal:
        return 0;
    case PM_TabBarTabHSpace:
        return PaddingHL + PaddingHL;
    case PM_TabBarTabVSpace: {
        const int textHeight = opt->fontMetrics.height();
        return PaddingVM + TabBarTf.lineHeight() + PaddingVM - textHeight;
    }
    default:
        return QCommonStyle::pixelMetric(m, opt, widget);
    }
}

QIcon QtDesignSystemStyle::standardIcon(StandardPixmap sp, const QStyleOption *opt,
                                        const QWidget *widget) const
{
    QIcon icon;

    switch (sp) {
    case SP_TabCloseButton: {
        static const QIcon tabClose = Icons::CLOSE_FOREGROUND.icon();
        icon = tabClose;
        break;
    }
    default:
        icon = QCommonStyle::standardIcon(sp, opt, widget);
        break;
    }

    return icon;
}
void QtDesignSystemStyle::polish(QWidget *widget)
{
    QCommonStyle::polish(widget);

    if (auto tabBar = qobject_cast<QTabBar*>(widget)) {
        tabBar->setFont(TabBarTf.font());
        for (int count = tabBar->count(), i = 0; i < count; ++i ) {
            for (const QTabBar::ButtonPosition pos : {QTabBar::LeftSide, QTabBar::RightSide}) {
                if (QWidget *tabButton = tabBar->tabButton(i, pos))
                    tabButton->setStyle(this);
            }
        }
    }
}

QStyle *QtDesignSystemStyle::instance()
{
    static QtDesignSystemStyle style;
    return &style;
}

} // namespace Utils
