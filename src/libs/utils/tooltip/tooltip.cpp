/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "tooltip.h"
#include "tips.h"
#include "tipcontents.h"
#include "effects.h"
#include "reuse.h"

#include <utils/hostosinfo.h>

#include <QString>
#include <QColor>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QMenu>

#include <QDebug>

using namespace Utils;
using namespace Internal;

ToolTip::ToolTip() : m_tip(0), m_widget(0)
{
    connect(&m_showTimer, SIGNAL(timeout()), this, SLOT(hideTipImmediately()));
    connect(&m_hideDelayTimer, SIGNAL(timeout()), this, SLOT(hideTipImmediately()));
}

ToolTip::~ToolTip()
{
    m_tip = 0;
}

ToolTip *ToolTip::instance()
{
    static ToolTip tooltip;
    return &tooltip;
}

void ToolTip::show(const QPoint &pos, const TipContent &content, QWidget *w, const QRect &rect)
{
    if (acceptShow(content, pos, w, rect)) {
        QWidget *target = 0;
        if (HostOsInfo::isWindowsHost())
            target = QApplication::desktop()->screen(Internal::screenNumber(pos, w));
        else
            target = w;

        switch (content.typeId()) {
            case TextContent::TEXT_CONTENT_ID:
                m_tip = new TextTip(target);
                break;
            case ColorContent::COLOR_CONTENT_ID:
                m_tip = new ColorTip(target);
                break;
            case WidgetContent::WIDGET_CONTENT_ID:
                m_tip = new WidgetTip(target);
                break;
        }
        setUp(pos, content, w, rect);
        qApp->installEventFilter(this);
        showTip();
    }
}

void ToolTip::show(const QPoint &pos, const TipContent &content, QWidget *w)
{
    show(pos, content, w, QRect());
}

bool ToolTip::acceptShow(const TipContent &content,
                         const QPoint &pos,
                         QWidget *w,
                         const QRect &rect)
{
    if (!validateContent(content))
        return false;

    if (isVisible()) {
        if (m_tip->canHandleContentReplacement(content)) {
            // Reuse current tip.
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (tipChanged(localPos, content, w))
                setUp(pos, content, w, rect);
            return false;
        }
        hideTipImmediately();
    }
#if !defined(QT_NO_EFFECTS) && !defined(Q_OS_MAC)
    // While the effect takes places it might be that although the widget is actually on
    // screen the isVisible method doesn't return true.
    else if (m_tip
             && (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)
                 || QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))) {
        hideTipImmediately();
    }
#endif
    return true;
}

bool ToolTip::validateContent(const TipContent &content)
{
    if (!content.isValid()) {
        if (isVisible())
            hideTipWithDelay();
        return false;
    }
    return true;
}

void ToolTip::setUp(const QPoint &pos, const TipContent &content, QWidget *w, const QRect &rect)
{
    m_tip->setContent(content);
    m_tip->configure(pos, w);

    placeTip(pos, w);
    setTipRect(w, rect);

    if (m_hideDelayTimer.isActive())
        m_hideDelayTimer.stop();
    m_showTimer.start(content.showTime());
}

bool ToolTip::tipChanged(const QPoint &pos, const TipContent &content, QWidget *w) const
{
    if (!m_tip->content().equals(content) || m_widget != w)
        return true;
    if (!m_rect.isNull())
        return !m_rect.contains(pos);
    return false;
}

void ToolTip::setTipRect(QWidget *w, const QRect &rect)
{
    if (!m_rect.isNull() && !w)
        qWarning("ToolTip::show: Cannot pass null widget if rect is set");
    else{
        m_widget = w;
        m_rect = rect;
    }
}

bool ToolTip::isVisible() const
{
    return m_tip && m_tip->isVisible();
}

void ToolTip::showTip()
{
#if !defined(QT_NO_EFFECTS) && !defined(Q_OS_MAC)
    if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip))
        qFadeEffect(m_tip);
    else if (QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))
        qScrollEffect(m_tip);
    else
        m_tip->show();
#else
    m_tip->show();
#endif
}

void ToolTip::hide()
{
    hideTipWithDelay();
}

void ToolTip::hideTipWithDelay()
{
    if (!m_hideDelayTimer.isActive())
        m_hideDelayTimer.start(300);
}

void ToolTip::hideTipImmediately()
{
    if (m_tip) {
        m_tip->close();
        m_tip->deleteLater();
        m_tip = 0;
    }
    m_showTimer.stop();
    m_hideDelayTimer.stop();
    qApp->removeEventFilter(this);
}

void ToolTip::placeTip(const QPoint &pos, QWidget *w)
{
    QRect screen = Internal::screenGeometry(pos, w);
    QPoint p = pos;
    p += QPoint(2, Utils::HostOsInfo::isWindowsHost() ? 21 : 16);
    if (p.x() + m_tip->width() > screen.x() + screen.width())
        p.rx() -= 4 + m_tip->width();
    if (p.y() + m_tip->height() > screen.y() + screen.height())
        p.ry() -= 24 + m_tip->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + m_tip->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - m_tip->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + m_tip->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - m_tip->height());

    m_tip->move(p);
}

bool ToolTip::eventFilter(QObject *o, QEvent *event)
{
    if (!o->isWidgetType())
        return false;

    switch (event->type()) {
#ifdef Q_OS_MAC
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        int key = static_cast<QKeyEvent *>(event)->key();
        Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(event)->modifiers();
        if (!(mody & Qt::KeyboardModifierMask)
            && key != Qt::Key_Shift && key != Qt::Key_Control
            && key != Qt::Key_Alt && key != Qt::Key_Meta)
            hideTipWithDelay();
        break;
    }
#endif
    case QEvent::Leave:
        if (o == m_tip)
            hideTipWithDelay();
        break;
    case QEvent::Enter:
        // User moved cursor into tip and wants to interact.
        if (m_tip && m_tip->isInteractive() && o == m_tip) {
            if (m_hideDelayTimer.isActive())
                m_hideDelayTimer.stop();
        }
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
    case QEvent::FocusIn:
        if (m_tip && !m_tip->isInteractive()) // Windows: A sequence of those occurs when interacting
            hideTipImmediately();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        if (m_tip) {
            if (m_tip->isInteractive()) { // Do not close on interaction with the tooltip
                if (o != m_tip && !m_tip->isAncestorOf(static_cast<QWidget *>(o)))
                    hideTipImmediately();
            } else {
                hideTipImmediately();
            }
        }
        break;
    case QEvent::MouseMove:
        if (o == m_widget &&
            !m_rect.isNull() &&
            !m_rect.contains(static_cast<QMouseEvent*>(event)->pos())) {
            hideTipWithDelay();
        }
    default:
        break;
    }
    return false;
}

QFont ToolTip::font() const
{
    return QApplication::font("QTipLabel");
}

void ToolTip::setFont(const QFont &font)
{
    QApplication::setFont(font, "QTipLabel");
}
