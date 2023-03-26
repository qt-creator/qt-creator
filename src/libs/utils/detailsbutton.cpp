// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "detailsbutton.h"

#include "hostosinfo.h"
#include "icon.h"
#include "stylehelper.h"
#include "utilstr.h"

#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>
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
    setText(Tr::tr("Details"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    if (HostOsInfo::isMacHost())
        setFont(QGuiApplication::font());
}

QSize DetailsButton::sizeHint() const
{
    const QSize textSize = fontMetrics().size(Qt::TextSingleLine, text());
    return QSize(spacing + textSize.width() + spacing + 16 + spacing,
                 spacing + fontMetrics().height() + spacing);
}

QColor DetailsButton::outlineColor()
{
    return HostOsInfo::isMacHost()
            ? QGuiApplication::palette().color(QPalette::Mid)
            : StyleHelper::mergedColors(creatorTheme()->color(Theme::TextColorNormal),
                                        creatorTheme()->color(Theme::BackgroundColorNormal), 15);
}

void DetailsButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter p(this);
    if (isEnabled() && (isChecked() || (!HostOsInfo::isMacHost() && underMouse()))) {
        p.save();
        p.setOpacity(0.125);
        p.fillRect(rect(), palette().color(QPalette::Text));
        p.restore();
    }

    if (!creatorTheme()->flag(Theme::FlatProjectsMode))
        qDrawPlainRect(&p, rect(), outlineColor());

    const QRect textRect(spacing + 3, 0, width(), height());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    if (creatorTheme()->flag(Theme::FlatProjectsMode) || HostOsInfo::isMacHost()) {
        const QRect iconRect(width() - spacing - 15, 0, 16, height());
        icon().paint(&p, iconRect);
    } else {
        int arrowsize = 15;
        QStyleOption arrowOpt;
        arrowOpt.initFrom(this);
        arrowOpt.rect = QRect(size().width() - arrowsize - spacing, height() / 2 - arrowsize / 2,
                              arrowsize, arrowsize);
        style()->drawPrimitive(isChecked() ? QStyle::PE_IndicatorArrowUp
                                           : QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
    }
}
