// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "indicatoractionwidget.h"

#include <theme.h>
#include <utils/icon.h>
#include <utils/stylehelper.h>

#include <QActionGroup>
#include <QMenu>
#include <QMouseEvent>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolBar>

namespace QmlDesigner {

static void drawIndicator(QPainter *painter, const QPoint &point, int dimension)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(Theme::getColor(Theme::DSamberLight));
    painter->drawEllipse(point, dimension, dimension);
    painter->restore();
}

IndicatorButton::IndicatorButton(QWidget *parent)
    : QToolButton(parent)
{
    Utils::StyleHelper::setPanelWidget(this);
    Utils::StyleHelper::setPanelWidgetSingleRow(this);
}

bool IndicatorButton::indicator() const
{
    return m_indicator;
}

void IndicatorButton::setIndicator(bool newIndicator)
{
    if (m_indicator != newIndicator) {
        m_indicator = newIndicator;
        emit indicatorChanged(m_indicator);
        update();
    }
}

QSize IndicatorButton::sizeHint() const
{
    if (QMenu *menu = qobject_cast<QMenu *>(parent())) {
        ensurePolished();
        QStyleOptionMenuItem opt;
        initMenuStyleOption(menu, &opt, defaultAction());
        QSize sz = style()
                       ->itemTextRect(fontMetrics(), QRect(), Qt::TextShowMnemonic, false, text())
                       .size();
        if (!opt.icon.isNull())
            sz = QSize(sz.width() + opt.maxIconWidth + 4, qMax(sz.height(), opt.maxIconWidth));
        QSize size = style()->sizeFromContents(QStyle::CT_MenuItem, &opt, sz, this);
        return size;
    }
    return Super::sizeHint();
}

void IndicatorButton::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    QStylePainter p(this);

    if (QMenu *menu = qobject_cast<QMenu *>(parent())) {
        this->setFixedWidth(menu->width());
        QStyleOptionMenuItem opt;
        initMenuStyleOption(menu, &opt, defaultAction());
        p.drawControl(QStyle::CE_MenuItem, opt);

        if (indicator() && opt.maxIconWidth && !opt.icon.isNull()) {
            const int indicatorDim = opt.rect.height() / 8;
            const int indicatorOffset = indicatorDim * 5 / 4;

            drawIndicator(&p,
                          opt.rect.topLeft()
                              + QPoint{opt.rect.height() - indicatorOffset, indicatorOffset},
                          indicatorDim);
        }
    } else {
        QStyleOptionToolButton option;
        initStyleOption(&option);
        p.drawComplexControl(QStyle::CC_ToolButton, option);

        if (indicator() && option.iconSize.isValid() && !option.icon.isNull()) {
            const int indicatorDim = std::min(option.rect.width(), option.rect.height()) / 8;
            const int indicatorOffset = indicatorDim * 5 / 4;
            drawIndicator(&p,
                          option.rect.topRight() + QPoint{-indicatorOffset, indicatorOffset},
                          indicatorDim);
        }
    }
}

void IndicatorButton::initMenuStyleOption(QMenu *menu,
                                          QStyleOptionMenuItem *option,
                                          const QAction *action) const
{
    if (!option || !action)
        return;

    option->initFrom(menu);
    option->palette = palette();
    option->state = QStyle::State_None;

    if (window()->isActiveWindow())
        option->state |= QStyle::State_Active;
    if (isEnabled() && action->isEnabled() && (!action->menu() || action->menu()->isEnabled()))
        option->state |= QStyle::State_Enabled;
    else
        option->palette.setCurrentColorGroup(QPalette::Disabled);

    option->font = action->font().resolve(font());
    option->fontMetrics = QFontMetrics(option->font);

    if (menu->activeAction() && menu->activeAction() == action)
        option->state |= QStyle::State_Selected;

    option->menuHasCheckableItems = false;
    if (!action->isCheckable()) {
        option->checkType = QStyleOptionMenuItem::NotCheckable;
    } else {
        option->checkType = (action->actionGroup() && action->actionGroup()->isExclusive())
                                ? QStyleOptionMenuItem::Exclusive
                                : QStyleOptionMenuItem::NonExclusive;
        option->checked = action->isChecked();
    }
    if (action->menu())
        option->menuItemType = QStyleOptionMenuItem::SubMenu;
    else if (action->isSeparator())
        option->menuItemType = QStyleOptionMenuItem::Separator;
    else
        option->menuItemType = QStyleOptionMenuItem::Normal;
    if (action->isIconVisibleInMenu())
        option->icon = action->icon();

    option->text = action->text();
    option->maxIconWidth = 20;
    option->rect = rect();
}

IndicatorButtonAction::IndicatorButtonAction(const QString &description,
                                             const QIcon &icon,
                                             QObject *parent)
    : QWidgetAction(parent)
{
    setText(description);
    setToolTip(description);
    setIcon(icon);
}

IndicatorButtonAction::~IndicatorButtonAction() = default;

void IndicatorButtonAction::setIndicator(bool indicator)
{
    if (m_indicator != indicator) {
        m_indicator = indicator;
        emit indicatorChanged(m_indicator, QPrivateSignal{});
    }
}

QWidget *IndicatorButtonAction::createWidget(QWidget *parent)
{
    if (qobject_cast<QMenu *>(parent))
        return nullptr;

    IndicatorButton *button = new IndicatorButton(parent);

    connect(this, &IndicatorButtonAction::indicatorChanged, button, &IndicatorButton::setIndicator);
    connect(button, &IndicatorButton::indicatorChanged, this, &IndicatorButtonAction::setIndicator);
    connect(button, &QToolButton::clicked, this, &QAction::trigger);
    button->setIndicator(m_indicator);
    button->setDefaultAction(this);

    if (QToolBar *tb = qobject_cast<QToolBar *>(parent)) {
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setIconSize(tb->iconSize());
        button->setToolButtonStyle(tb->toolButtonStyle());
        connect(tb, &QToolBar::iconSizeChanged, button, &IndicatorButton::setIconSize);
        connect(tb, &QToolBar::toolButtonStyleChanged, button, &IndicatorButton::setToolButtonStyle);
        connect(button, &IndicatorButton::triggered, tb, &QToolBar::actionTriggered);
    }

    return button;
}

} // namespace QmlDesigner
