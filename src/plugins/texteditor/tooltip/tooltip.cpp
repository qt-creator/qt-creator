/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "tooltip.h"
#include "tips.h"
#include "tipcontents.h"

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QToolTip>
#include <QtGui/QKeyEvent>

using namespace TextEditor;
using namespace Internal;

ToolTip::ToolTip() : m_tip(0), m_widget(0)
{
    connect(&m_showTimer, SIGNAL(timeout()), this, SLOT(hideTipImmediately()));
    connect(&m_hideDelayTimer, SIGNAL(timeout()), this, SLOT(hideTipImmediately()));
}

ToolTip::~ToolTip()
{ m_tip = 0; }

ToolTip *ToolTip::instance()
{
    static ToolTip tooltip;
    return &tooltip;
}

void ToolTip::showText(const QPoint &pos, const QString &text, QWidget *w)
{
    hideTipImmediately();
    QToolTip::showText(pos, text, w);
}

void ToolTip::showColor(const QPoint &pos, const QColor &color, QWidget *w)
{
    hideQtTooltip();
    QSharedPointer<TipContent> colorContent(new QColorContent(color));
    if (acceptShow(colorContent, pos, w) && colorContent->isValid()) {
#ifndef Q_WS_WIN
        m_tip = new ColorTip(w);
#else
        m_tip = new ColorTip(QApplication::desktop()->screen(tipScreen(pos, w)));
#endif
        setUp(colorContent, pos, w);
        qApp->installEventFilter(this);
        showTip();
    }
}

bool ToolTip::isVisible() const
{
    return QToolTip::isVisible() || (m_tip && m_tip->isVisible());
}

bool ToolTip::acceptShow(const QSharedPointer<TipContent> &content, const QPoint &pos, QWidget *w)
{
    if (m_tip && m_tip->isVisible()) {
        if (!content->isValid()) {
            hideTipWithDelay();
            return false;
        } else {
            // Reuse current tip.
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (requiresSetUp(content, w))
                setUp(content, pos, w);
            return false;
        }
    }
    return true;
}

void ToolTip::setUp(const QSharedPointer<TipContent> &content, const QPoint &pos, QWidget *w)
{
    m_tip->setContent(content);
    placeTip(pos, w);
    m_widget = w;
    m_showTimer.start(content->showTime());
}

bool ToolTip::requiresSetUp(const QSharedPointer<TipContent> &tipContent, QWidget *w) const
{
    if (!m_tip->content()->equals(tipContent.data()))
        return true;
    if (m_widget != w)
        return true;
    return false;
}

void ToolTip::showTip()
{
    m_tip->show();
}

void ToolTip::hide()
{
    hideQtTooltip();
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

void ToolTip::hideQtTooltip()
{
    if (QToolTip::isVisible())
        QToolTip::hideText();
}

void ToolTip::placeTip(const QPoint &pos, QWidget *w)
{
#ifdef Q_WS_MAC
    QRect screen = QApplication::desktop()->availableGeometry(tipScreen(pos, w));
#else
    QRect screen = QApplication::desktop()->screenGeometry(tipScreen(pos, w));
#endif

    QPoint p = pos;
    p += QPoint(2,
#ifdef Q_WS_WIN
                21
#else
                16
#endif
                );

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

int ToolTip::tipScreen(const QPoint &pos, QWidget *w) const
{
    if (QApplication::desktop()->isVirtualDesktop())
        return QApplication::desktop()->screenNumber(pos);
    else
        return QApplication::desktop()->screenNumber(w);
}

bool ToolTip::eventFilter(QObject *o, QEvent *event)
{
    Q_UNUSED(o)

    switch (event->type()) {
#ifdef Q_WS_MAC
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
        hideTipWithDelay();
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Wheel:
        hideTipImmediately();
        break;

    default:
        break;
    }
    return false;
}
