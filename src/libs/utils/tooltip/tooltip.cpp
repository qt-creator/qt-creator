// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tooltip.h"
#include "tips.h"
#include "effects.h"

#include "faketooltip.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QApplication>
#include <QColor>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QScreen>
#include <QWidget>

using namespace Utils;
using namespace Internal;

ToolTip::ToolTip() : m_tip(nullptr), m_widget(nullptr)
{
    connect(&m_showTimer, &QTimer::timeout, this, &ToolTip::hideTipImmediately);
    connect(&m_hideDelayTimer, &QTimer::timeout, this, &ToolTip::hideTipImmediately);
}

ToolTip::~ToolTip()
{
    m_tip = nullptr;
}

ToolTip *ToolTip::instance()
{
    static ToolTip tooltip;
    return &tooltip;
}

static QWidget *createF1Icon()
{
    auto label = new QLabel;
    label->setPixmap({":/utils/tooltip/images/f1.png"});
    label->setAlignment(Qt::AlignTop);
    return label;
}

/*!
    Shows a tool tip with the text \a content. If \a contextHelp is given, a context help icon
    is shown as well.
    \a contextHelp of the current shown tool tip can be retrieved via ToolTip::contextHelp().
*/
void ToolTip::show(const QPoint &pos, const QString &content, QWidget *w, const QVariant &contextHelp, const QRect &rect)
{
    show(pos, content, Qt::AutoText, w, contextHelp, rect);
}

/*!
    Shows a tool tip with the text \a content with a specific text \a format.
    If \a contextHelp is given, a context help icon is shown as well.
    \a contextHelp of the current shown tool tip can be retrieved via ToolTip::contextHelp().
*/
void ToolTip::show(const QPoint &pos,
                   const QString &content,
                   Qt::TextFormat format,
                   QWidget *w,
                   const QVariant &contextHelp,
                   const QRect &rect)
{
    if (content.isEmpty()) {
        instance()->hideTipWithDelay();
    } else {
        if (contextHelp.isNull()) {
            instance()->showInternal(pos,
                                     QVariant::fromValue(TextItem(content, format)),
                                     TextContent,
                                     w,
                                     contextHelp,
                                     rect);
        } else {
            auto tooltipWidget = new FakeToolTip;
            auto layout = new QHBoxLayout;
            layout->setContentsMargins(0, 0, 0, 0);
            tooltipWidget->setLayout(layout);
            auto label = new QLabel;
            label->setObjectName("qcWidgetTipTopLabel");
            label->setTextFormat(format);
            label->setText(content);
            layout->addWidget(label);
            layout->addWidget(createF1Icon());
            instance()->showInternal(pos,
                                     QVariant::fromValue(tooltipWidget),
                                     WidgetContent,
                                     w,
                                     contextHelp,
                                     rect);
        }
    }
}

/*!
    Shows a tool tip with a small rectangle in the given \a color.
    \a contextHelp of the current shown tool tip can be retrieved via ToolTip::contextHelp().
*/
void ToolTip::show(const QPoint &pos, const QColor &color, QWidget *w, const QVariant &contextHelp, const QRect &rect)
{
    if (!color.isValid())
        instance()->hideTipWithDelay();
    else
        instance()->showInternal(pos, QVariant(color), ColorContent, w, contextHelp, rect);
}

/*!
    Shows the widget \a content as a tool tip. The tool tip takes ownership of the widget.
    \a contextHelp of the current shown tool tip can be retrieved via ToolTip::contextHelp().
*/
void ToolTip::show(const QPoint &pos, QWidget *content, QWidget *w, const QVariant &contextHelp, const QRect &rect)
{
    if (!content)
        instance()->hideTipWithDelay();
    else
        instance()->showInternal(pos, QVariant::fromValue(content), WidgetContent, w, contextHelp, rect);
}

/*!
    Shows the layout \a content as a tool tip. The tool tip takes ownership of the layout.
    If \a contextHelp is given, a context help icon is shown as well.
    \a contextHelp of the current shown tool tip can be retrieved via ToolTip::contextHelp().
*/
void ToolTip::show(
    const QPoint &pos, QLayout *content, QWidget *w, const QVariant &contextHelp, const QRect &rect)
{
    if (content && content->count()) {
        auto tooltipWidget = new FakeToolTip;
        // limit the size of the widget to 90% of the screen size to have some context around it
        QScreen *qscreen = QGuiApplication::screenAt(pos);
        if (!qscreen)
            qscreen = QGuiApplication::primaryScreen();
        tooltipWidget->setMaximumSize(qscreen->availableSize() * 0.9);
        if (contextHelp.isNull()) {
            tooltipWidget->setLayout(content);
        } else {
            auto layout = new QHBoxLayout;
            layout->setContentsMargins(0, 0, 0, 0);
            tooltipWidget->setLayout(layout);
            layout->addLayout(content);
            layout->addWidget(createF1Icon());
        }
        instance()->showInternal(pos, QVariant::fromValue(tooltipWidget), WidgetContent, w, contextHelp, rect);
    } else {
        instance()->hideTipWithDelay();
    }
}

void ToolTip::move(const QPoint &pos)
{
    if (isVisible())
        instance()->placeTip(pos);
}

bool ToolTip::pinToolTip(QWidget *w, QWidget *parent)
{
    QTC_ASSERT(w, return false);
    // Find the parent WidgetTip, tell it to pin/release the
    // widget and close.
    for (QWidget *p = w->parentWidget(); p ; p = p->parentWidget()) {
        if (auto wt = qobject_cast<WidgetTip *>(p)) {
            wt->pinToolTipWidget(parent);
            ToolTip::hide();
            return true;
        }
    }
    return false;
}

QVariant ToolTip::contextHelp()
{
    return instance()->m_tip ? instance()->m_tip->contextHelp() : QVariant();
}

bool ToolTip::acceptShow(const QVariant &content,
                         int typeId,
                         const QPoint &pos,
                         QWidget *w, const QVariant &contextHelp,
                         const QRect &rect)
{
    if (isVisible()) {
        if (m_tip->canHandleContentReplacement(typeId)) {
            // Reuse current tip.
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (tipChanged(localPos, content, typeId, w, contextHelp)) {
                m_tip->setContent(content);
                m_tip->setContextHelp(contextHelp);
                setUp(pos, w, rect);
            }
            return false;
        }
        hideTipImmediately();
    }
#if !defined(QT_NO_EFFECTS) && !defined(Q_OS_MAC)
    // While the effect takes places it might be that although the widget is actually on
    // screen the isVisible function doesn't return true.
    else if (m_tip
             && (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)
                 || QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))) {
        hideTipImmediately();
    }
#endif
    return true;
}

void ToolTip::setUp(const QPoint &pos, QWidget *w, const QRect &rect)
{
    m_tip->configure(pos);

    placeTip(pos);
    setTipRect(w, rect);

    if (m_hideDelayTimer.isActive())
        m_hideDelayTimer.stop();
    m_showTimer.start(m_tip->showTime());
}

bool ToolTip::tipChanged(const QPoint &pos, const QVariant &content, int typeId, QWidget *w,
                         const QVariant &contextHelp) const
{
    if (!m_tip->equals(typeId, content, contextHelp) || m_widget != w)
        return true;
    if (!m_rect.isNull())
        return !m_rect.contains(pos);
    return false;
}

void ToolTip::setTipRect(QWidget *w, const QRect &rect)
{
    if (!m_rect.isNull() && !w) {
        qWarning("ToolTip::show: Cannot pass null widget if rect is set");
    } else {
        m_widget = w;
        m_rect = rect;
    }
}

bool ToolTip::isVisible()
{
    ToolTip *t = instance();
    return t->m_tip && t->m_tip->isVisible();
}

QPoint ToolTip::offsetFromPosition()
{
    return QPoint(2, HostOsInfo::isWindowsHost() ? 21 : 16);
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
    instance()->hideTipWithDelay();
}

void ToolTip::hideImmediately()
{
    instance()->hideTipImmediately();
}

void ToolTip::hideTipWithDelay()
{
    if (!m_hideDelayTimer.isActive())
        m_hideDelayTimer.start(300);
}

void ToolTip::hideTipImmediately()
{
    if (m_tip) {
        TipLabel *tip = m_tip.data();
        m_tip.clear();

        tip->close();
        tip->deleteLater();
    }
    m_showTimer.stop();
    m_hideDelayTimer.stop();
    qApp->removeEventFilter(this);
    emit hidden();
}

void ToolTip::showInternal(const QPoint &pos, const QVariant &content,
                           int typeId, QWidget *w, const QVariant &contextHelp, const QRect &rect)
{
    if (acceptShow(content, typeId, pos, w, contextHelp, rect)) {
        switch (typeId) {
            case ColorContent:
                m_tip = new ColorTip(w);
                break;
            case TextContent:
                m_tip = new TextTip(w);
                break;
            case WidgetContent:
                m_tip = new WidgetTip(w);
                break;
        }
        m_tip->setObjectName("qcToolTip");
        m_tip->setContent(content);
        m_tip->setContextHelp(contextHelp);
        setUp(pos, w, rect);
        qApp->installEventFilter(this);
        showTip();
    }
    emit shown();
}

void ToolTip::placeTip(const QPoint &pos)
{
    QScreen *qscreen = QGuiApplication::screenAt(pos);
    if (!qscreen)
        qscreen = QGuiApplication::primaryScreen();
    const QRect screen = qscreen->availableGeometry();
    QPoint p = pos;
    p += offsetFromPosition();
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
    if (m_tip && event->type() == QEvent::ApplicationStateChange
            && QGuiApplication::applicationState() != Qt::ApplicationActive) {
        hideTipImmediately();
    }

    if (!o->isWidgetType())
        return false;

    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        int key = static_cast<QKeyEvent *>(event)->key();
        if (key == Qt::Key_Escape)
            hideTipImmediately();
        if (HostOsInfo::isMacHost()) {
            Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(event)->modifiers();
            if (!(mody & Qt::KeyboardModifierMask)
                && key != Qt::Key_Shift && key != Qt::Key_Control
                && key != Qt::Key_Alt && key != Qt::Key_Meta)
                hideTipWithDelay();
        }
        break;
    }
    case QEvent::Leave:
        if (o == m_tip && !m_tip->isAncestorOf(QApplication::focusWidget()))
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
        break;
    default:
        break;
    }
    return false;
}
