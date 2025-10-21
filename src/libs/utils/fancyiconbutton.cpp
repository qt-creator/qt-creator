// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fancyiconbutton.h"
#include "hostosinfo.h"
#include "stylehelper.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QStylePainter>
#include <QWindow>

namespace Utils {

FancyIconButton::FancyIconButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

void FancyIconButton::paintEvent(QPaintEvent *)
{
    const qreal pixelRatio = window()->windowHandle()->devicePixelRatio();
    const QPixmap iconPixmap = icon().pixmap(sizeHint(), pixelRatio,
                                             isEnabled() ? QIcon::Normal : QIcon::Disabled);
    QStylePainter painter(this);
    QRect pixmapRect(QPoint(), iconPixmap.size() / iconPixmap.devicePixelRatio());
    pixmapRect.moveCenter(rect().center());

    if (m_autoHide)
        painter.setOpacity(m_iconOpacity);

    painter.drawPixmap(pixmapRect, iconPixmap);

    if (hasFocus()) {
        QStyleOptionFocusRect focusOption;
        focusOption.initFrom(this);
        focusOption.rect = pixmapRect;
        if (HostOsInfo::isMacHost()) {
            focusOption.rect.adjust(-4, -4, 4, 4);
            painter.drawControl(QStyle::CE_FocusFrame, focusOption);
        } else {
            painter.drawPrimitive(QStyle::PE_FrameFocusRect, focusOption);
        }
    }
}

void FancyIconButton::animateShow(bool visible)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "iconOpacity");
    animation->setDuration(StyleHelper::defaultFadeAnimationDuration);
    animation->setEndValue(visible ? 1.0 : 0.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QSize FancyIconButton::sizeHint() const
{
    return icon().actualSize(QSize(32, 16)); // Find flags icon can be wider than 16px
}

void FancyIconButton::keyPressEvent(QKeyEvent *ke)
{
    QAbstractButton::keyPressEvent(ke);
    if (!ke->modifiers() && (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return))
        click();
    // do not forward to line edit
    ke->accept();
}

void FancyIconButton::keyReleaseEvent(QKeyEvent *ke)
{
    QAbstractButton::keyReleaseEvent(ke);
    // do not forward to line edit
    ke->accept();
}

} // namespace Utils
