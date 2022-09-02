// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "detailsbutton.h"

#include <utils/hostosinfo.h>
#include <utils/icon.h>

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include <qdrawutil.h>

using namespace Utils;

FadingWidget::FadingWidget(QWidget *parent) :
    FadingPanel(parent),
    m_opacityEffect(new QGraphicsOpacityEffect)
{
    m_opacityEffect->setOpacity(0);
    setGraphicsEffect(m_opacityEffect);

    // Workaround for issue with QGraphicsEffect. GraphicsEffect
    // currently clears with Window color. Remove if flickering
    // no longer occurs on fade-in
    QPalette pal;
    pal.setBrush(QPalette::All, QPalette::Window, Qt::transparent);
    setPalette(pal);
}

void FadingWidget::setOpacity(qreal value)
{
    m_opacityEffect->setOpacity(value);
}

void FadingWidget::fadeTo(qreal value)
{
    QPropertyAnimation *animation = new QPropertyAnimation(m_opacityEffect, "opacity");
    animation->setDuration(200);
    animation->setEndValue(value);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

qreal FadingWidget::opacity()
{
    return m_opacityEffect->opacity();
}

ExpandButton::ExpandButton(QWidget *parent)
    : QToolButton(parent)
{
    setCheckable(true);
    auto updateArrow = [this] (bool checked) {
        static const QIcon expand =
                Icon({{":/utils/images/arrowdown.png", Theme::PanelTextColorDark}}, Icon::Tint).icon();
        static const QIcon collapse =
                Icon({{":/utils/images/arrowup.png", Theme::PanelTextColorDark}}, Icon::Tint).icon();
        setIcon(checked ? collapse : expand);
    };
    updateArrow(false);
    connect(this, &QToolButton::toggled, this, updateArrow);
}

DetailsButton::DetailsButton(QWidget *parent)
    : ExpandButton(parent)
{
    setText(tr("Details"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
}

QSize DetailsButton::sizeHint() const
{
    const QSize textSize = fontMetrics().size(Qt::TextSingleLine, text());
    return QSize(spacing + textSize.width() + spacing + 16 + spacing,
                 spacing + fontMetrics().height() + spacing);
}

void DetailsButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter p(this);
    if (isChecked() || (!HostOsInfo::isMacHost() && underMouse())) {
        p.save();
        p.setOpacity(0.125);
        p.fillRect(rect(), palette().color(QPalette::Text));
        p.restore();
    }

    if (!creatorTheme()->flag(Theme::FlatProjectsMode))
        qDrawPlainRect(&p, rect(), palette().color(QPalette::Mid));

    const QRect textRect(spacing, 0, width(), height());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    const QRect iconRect(width() - spacing - 16, 0, 16, height());
    icon().paint(&p, iconRect);
}
