// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "faketooltip.h"

#include <QStyleOption>
#include <QStylePainter>

/*!
    \class Utils::FakeToolTip
    \inmodule QtCreator

    \brief The FakeToolTip class is a widget that pretends to be a tooltip.

    By default it has Qt::WA_DeleteOnClose set.
*/

namespace Utils {

FakeToolTip::FakeToolTip(QWidget *parent) :
    QWidget(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_DeleteOnClose);

    // Set the window and button text to the tooltip text color, since this
    // widget draws the background as a tooltip.
    QPalette p = palette();
    const QColor toolTipTextColor = p.color(QPalette::Inactive, QPalette::ToolTipText);
    p.setColor(QPalette::Inactive, QPalette::WindowText, toolTipTextColor);
    p.setColor(QPalette::Inactive, QPalette::ButtonText, toolTipTextColor);
    setPalette(p);

    const int margin = 1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this);
    setContentsMargins(margin + 1, margin, margin, margin);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
}

void FakeToolTip::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
}

void FakeToolTip::resizeEvent(QResizeEvent *)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.initFrom(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);
}

} // namespace Utils
