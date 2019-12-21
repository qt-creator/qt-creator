/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "infolabel.h"

#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <QPainter>
#include <QPaintEvent>
#include <QLabel>

namespace Utils {

constexpr int iconSize = 16;

InfoLabel::InfoLabel(QWidget *parent)
    : InfoLabel({}, Information, parent)
{
}

InfoLabel::InfoLabel(const QString &text, InfoType type, QWidget *parent)
    : ElidingLabel(text, parent)
{
    setType(type);
}

InfoLabel::InfoType InfoLabel::type() const
{
    return m_type;
}

void InfoLabel::setType(InfoType type)
{
    m_type = type;
    setContentsMargins(m_type == None ? 0 : iconSize + 2, 0, 0, 0);
    update();
}

bool InfoLabel::filled() const
{
    return m_filled;
}

void InfoLabel::setFilled(bool filled)
{
    m_filled = filled;
}

QSize InfoLabel::minimumSizeHint() const
{
    QSize baseHint = ElidingLabel::minimumSizeHint();
    baseHint.setHeight(qMax(baseHint.height(), iconSize));
    return baseHint;
}

static Utils::Theme::Color fillColorForType(InfoLabel::InfoType type)
{
    using namespace Utils;
    switch (type) {
    case InfoLabel::Warning:
        return Theme::IconsWarningColor;
    case InfoLabel::Ok:
        return Theme::IconsRunColor;
    case InfoLabel::Error:
    case InfoLabel::NotOk:
        return Theme::IconsErrorColor;
    case InfoLabel::Information:
    default:
        return Theme::IconsInfoColor;
    }
}

static const QIcon &iconForType(InfoLabel::InfoType type)
{
    using namespace Utils;
    switch (type) {
    case InfoLabel::Information: {
        static const QIcon icon = Icons::INFO.icon();
        return icon;
    }
    case InfoLabel::Warning: {
        static const QIcon icon = Icons::WARNING.icon();
        return icon;
    }
    case InfoLabel::Error: {
        static const QIcon icon = Icons::CRITICAL.icon();
        return icon;
    }
    case InfoLabel::Ok: {
        static const QIcon icon = Icons::OK.icon();
        return icon;
    }
    case InfoLabel::NotOk: {
        static const QIcon icon = Icons::BROKEN.icon();
        return icon;
    }
    default: {
        static const QIcon undefined;
        return undefined;
    }
    }
}

void InfoLabel::paintEvent(QPaintEvent *event)
{
    if (m_type == None)
        return ElidingLabel::paintEvent(event);

    const bool centerIconVertically = wordWrap() || elideMode() == Qt::ElideNone;
    const QRect iconRect(0, centerIconVertically ? 0 : ((height() - iconSize) / 2),
                         iconSize, iconSize);

    QPainter p(this);
    if (m_filled && isEnabled()) {
        p.save();
        p.setOpacity(0.175);
        p.fillRect(rect(), creatorTheme()->color(fillColorForType(m_type)));
        p.restore();
    }
    const QIcon &icon = iconForType(m_type);
    QWindow *window = this->window()->windowHandle();
    const QIcon::Mode mode = !this->isEnabled() ? QIcon::Disabled : QIcon::Normal;
    const QPixmap iconPx =
            icon.pixmap(window, QSize(iconSize, iconSize) * devicePixelRatio(), mode);
    p.drawPixmap(iconRect, iconPx);
    ElidingLabel::paintEvent(event);
}

} // namespace Utils
