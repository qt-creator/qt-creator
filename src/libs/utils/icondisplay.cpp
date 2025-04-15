// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icondisplay.h"

#include "icon.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace Utils
{

class IconDisplayPrivate
{
public:
    QIcon m_icon;
};

Utils::IconDisplay::IconDisplay(QWidget *parent) :
    QWidget(parent),
    d(std::make_unique<IconDisplayPrivate>())
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

IconDisplay::~IconDisplay() = default;

void IconDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    qreal dpr = devicePixelRatio();
    QSize actualSize = size();
    auto px = d->m_icon.pixmap(actualSize, dpr);
    px.setDevicePixelRatio(dpr);

    QRect r(QPoint(0, 0), actualSize);
    r.moveCenter(rect().center());

    painter.drawPixmap(r, px);
}

QSize IconDisplay::sizeHint() const
{
    if (d->m_icon.isNull())
        return {};

    if (auto sizes = d->m_icon.availableSizes(); !sizes.isEmpty())
        return sizes.first();

    return {};
}

void IconDisplay::setIcon(const Icon &icon)
{
    d->m_icon = icon.icon();
    update();
}

} // namespace Utils
