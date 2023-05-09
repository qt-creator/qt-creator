// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "styledbar.h"

#include "stylehelper.h"

#include <QPainter>
#include <QStyleOption>

using namespace Utils;

StyledBar::StyledBar(QWidget *parent)
    : QWidget(parent)
{
    StyleHelper::setPanelWidget(this);
    StyleHelper::setPanelWidgetSingleRow(this);
    setProperty(StyleHelper::C_LIGHT_COLORED, false);
}

void StyledBar::setSingleRow(bool singleRow)
{
    StyleHelper::setPanelWidgetSingleRow(this, singleRow);
}

bool StyledBar::isSingleRow() const
{
    return property(StyleHelper::C_PANEL_WIDGET_SINGLE_ROW).toBool();
}

void StyledBar::setLightColored(bool lightColored)
{
    if (isLightColored() == lightColored)
        return;
    setProperty(StyleHelper::C_LIGHT_COLORED, lightColored);
    const QList<QWidget *> children = findChildren<QWidget *>();
    for (QWidget *childWidget : children)
        childWidget->style()->polish(childWidget);
}

bool StyledBar::isLightColored() const
{
    return property(StyleHelper::C_LIGHT_COLORED).toBool();
}

void StyledBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QStyleOptionToolBar option;
    option.rect = rect();
    option.state = QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_ToolBar, &option, &painter, this);
}

StyledSeparator::StyledSeparator(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(10);
}

void StyledSeparator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QStyleOption option;
    option.rect = rect();
    option.state = QStyle::State_Horizontal;
    option.palette = palette();
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter, this);
}
