/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "progressbar.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QPropertyAnimation>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QMouseEvent>

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

static const int PROGRESSBAR_HEIGHT = 13;
static const int CANCELBUTTON_WIDTH = 16;
static const int SEPARATOR_HEIGHT = 2;

ProgressBar::ProgressBar(QWidget *parent)
    : QWidget(parent), m_titleVisible(true), m_separatorVisible(true), m_cancelEnabled(true),
      m_progressHeight(0),
      m_minimum(1), m_maximum(100), m_value(1), m_cancelButtonFader(0), m_finished(false),
      m_error(false)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setMouseTracking(true);
}

ProgressBar::~ProgressBar()
{
}

bool ProgressBar::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "cancelButtonFader");
            animation->setDuration(125);
            animation->setEndValue(1.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    case QEvent::Leave:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "cancelButtonFader");
            animation->setDuration(225);
            animation->setEndValue(0.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    default:
        return QWidget::event(e);
    }
    return false;
}


void ProgressBar::reset()
{
    m_value = m_minimum;
    update();
}

void ProgressBar::setRange(int minimum, int maximum)
{
    m_minimum = minimum;
    m_maximum = maximum;
    if (m_value < m_minimum || m_value > m_maximum)
        m_value = m_minimum;
    update();
}

void ProgressBar::setValue(int value)
{
    if (m_value == value
            || m_value < m_minimum
            || m_value > m_maximum) {
        return;
    }
    m_value = value;
    update();
}

void ProgressBar::setFinished(bool b)
{
    if (b == m_finished)
        return;
    m_finished = b;
    update();
}

QString ProgressBar::title() const
{
    return m_title;
}

bool ProgressBar::hasError() const
{
    return m_error;
}

void ProgressBar::setTitle(const QString &title)
{
    m_title = title;
    updateGeometry();
    update();
}

void ProgressBar::setTitleVisible(bool visible)
{
    if (m_titleVisible == visible)
        return;
    m_titleVisible = visible;
    updateGeometry();
    update();
}

bool ProgressBar::isTitleVisible() const
{
    return m_titleVisible;
}

void ProgressBar::setSeparatorVisible(bool visible)
{
    if (m_separatorVisible == visible)
        return;
    m_separatorVisible = visible;
    update();
}

bool ProgressBar::isSeparatorVisible() const
{
    return m_separatorVisible;
}

void ProgressBar::setCancelEnabled(bool enabled)
{
    if (m_cancelEnabled == enabled)
        return;
    m_cancelEnabled = enabled;
    update();
}

bool ProgressBar::isCancelEnabled() const
{
    return m_cancelEnabled;
}

void ProgressBar::setError(bool on)
{
    m_error = on;
    update();
}

QSize ProgressBar::sizeHint() const
{
    int width = 50;
    int height = PROGRESSBAR_HEIGHT + 5;
    if (m_titleVisible) {
        QFontMetrics fm(titleFont());
        width = qMax(width, fm.width(m_title) + 16);
        height += fm.height() + 5;
    }
    if (m_separatorVisible)
        height += SEPARATOR_HEIGHT;
    return QSize(width, height);
}

namespace { const int INDENT = 6; }

void ProgressBar::mousePressEvent(QMouseEvent *event)
{
    if (m_cancelEnabled) {
        if (event->modifiers() == Qt::NoModifier
            && m_cancelRect.contains(event->pos())) {
            event->accept();
            emit clicked();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

QFont ProgressBar::titleFont() const
{
    QFont boldFont(font());
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    return boldFont;
}

void ProgressBar::mouseMoveEvent(QMouseEvent *)
{
    update();
}

void ProgressBar::paintEvent(QPaintEvent *)
{
    // TODO move font into Utils::StyleHelper
    // TODO use Utils::StyleHelper white

    double range = maximum() - minimum();
    double percent = 0.;
    if (!qFuzzyIsNull(range))
        percent = qBound(0., (value() - minimum()) / range, 1.);

    if (finished())
        percent = 1;

    QPainter p(this);
    QFont fnt(titleFont());
    p.setFont(fnt);
    QFontMetrics fm(fnt);

    int titleHeight = m_titleVisible ? fm.height() : 0;

    // Draw separator
    int separatorHeight = m_separatorVisible ? SEPARATOR_HEIGHT : 0;
    if (m_separatorVisible) {
        QRectF innerRect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(StyleHelper::sidebarShadow());
        p.drawLine(innerRect.topLeft(), innerRect.topRight());

        if (creatorTheme()->flag(Theme::DrawToolBarHighlights)) {
            p.setPen(StyleHelper::sidebarHighlight());
            p.drawLine(innerRect.topLeft() + QPointF(1, 1), innerRect.topRight() + QPointF(0, 1));
        }
    }

    if (m_titleVisible) {
        QRect textBounds = fm.boundingRect(m_title);
        textBounds.moveCenter(rect().center());
        int alignment = Qt::AlignHCenter;

        int textSpace = rect().width() - 8;
        // If there is not enough room when centered, we left align and
        // elide the text
        QString elidedtitle  = fm.elidedText(m_title, Qt::ElideRight, textSpace);

        QRect textRect = rect().adjusted(3, separatorHeight - 1, -3, 0);
        textRect.setHeight(titleHeight + 4);

        p.setPen(creatorTheme()->color(Theme::ProgressBarTitleColor));
        p.drawText(textRect, alignment | Qt::AlignBottom, elidedtitle);
    }

    m_progressHeight = PROGRESSBAR_HEIGHT;
    m_progressHeight += ((m_progressHeight % 2) + 1) % 2; // make odd
    // draw outer rect
    const QRect rect(INDENT - 1, titleHeight + separatorHeight + (m_titleVisible ? 5 : 4),
                     size().width() - 2 * INDENT + 1, m_progressHeight);

    QRectF inner = rect.adjusted(2, 2, -2, -2);
    inner.adjust(0, 0, qRound((percent - 1) * inner.width()), 0);

    // Show at least a hint of progress. Non-flat needs more pixels due to the borders.
    inner.setWidth(qMax(qMin(3.0, qreal(rect.width())), inner.width()));

    Theme::Color themeColor = Theme::ProgressBarColorNormal;
    if (m_error)
        themeColor = Theme::ProgressBarColorError;
    else if (m_finished)
        themeColor = Theme::ProgressBarColorFinished;
    const QColor c = creatorTheme()->color(themeColor);

    //draw the progress bar
    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        p.fillRect(rect.adjusted(2, 2, -2, -2),
                   creatorTheme()->color(Theme::ProgressBarBackgroundColor));
        p.fillRect(inner, c);
    } else {
        const static QImage bar(StyleHelper::dpiSpecificImageFile(
                                    QLatin1String(":/utils/images/progressbar.png")));
        StyleHelper::drawCornerImage(bar, &p, rect, 3, 3, 3, 3);

        // Draw line and shadow after the gradient fill
        if (value() > 0 && value() < maximum()) {
            p.fillRect(QRect(inner.right(), inner.top(), 2, inner.height()), QColor(0, 0, 0, 20));
            p.fillRect(QRect(inner.right(), inner.top(), 1, inner.height()), QColor(0, 0, 0, 60));
        }

        QLinearGradient grad(inner.topLeft(), inner.bottomLeft());
        grad.setColorAt(0, c.lighter(130));
        grad.setColorAt(0.4, c.lighter(106));
        grad.setColorAt(0.41, c.darker(106));
        grad.setColorAt(1, c.darker(130));
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRect(inner);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0, 0, 0, 30), 1));

        p.drawLine(inner.topLeft() + QPointF(0.5, 0.5), inner.topRight() + QPointF(-0.5, 0.5));
        p.drawLine(inner.topLeft() + QPointF(0.5, 0.5), inner.bottomLeft() + QPointF(0.5, -0.5));
        p.drawLine(inner.topRight() + QPointF(-0.5, 0.5), inner.bottomRight() + QPointF(-0.5, -0.5));
        p.drawLine(inner.bottomLeft() + QPointF(0.5, -0.5), inner.bottomRight() + QPointF(-0.5, -0.5));
    }

    if (m_cancelEnabled) {
        // Draw cancel button
        p.setOpacity(m_cancelButtonFader);

        if (value() < maximum() && !m_error) {
            m_cancelRect = QRect(rect.adjusted(rect.width() - CANCELBUTTON_WIDTH + 2, 1, 0, 0));
            const bool hover = m_cancelRect.contains(mapFromGlobal(QCursor::pos()));
            const QRectF cancelVisualRect(m_cancelRect.adjusted(0, 1, -2, -2));
            int intensity = hover ? 90 : 70;
            if (!creatorTheme()->flag(Theme::FlatToolBars)) {
                QLinearGradient grad(cancelVisualRect.topLeft(), cancelVisualRect.bottomLeft());
                QColor buttonColor(intensity, intensity, intensity, 255);
                grad.setColorAt(0, buttonColor.lighter(130));
                grad.setColorAt(1, buttonColor.darker(130));
                p.setPen(Qt::NoPen);
                p.setBrush(grad);
                p.drawRect(cancelVisualRect);

                p.setPen(QPen(QColor(0, 0, 0, 30)));
                p.drawLine(cancelVisualRect.topLeft() + QPointF(-0.5, 0.5), cancelVisualRect.bottomLeft() + QPointF(-0.5, -0.5));
                p.setPen(QPen(QColor(0, 0, 0, 120)));
                p.drawLine(cancelVisualRect.topLeft() + QPointF(0.5, 0.5), cancelVisualRect.bottomLeft() + QPointF(0.5, -0.5));
                p.setPen(QPen(QColor(255, 255, 255, 30)));
                p.drawLine(cancelVisualRect.topLeft() + QPointF(1.5, 0.5), cancelVisualRect.bottomLeft() + QPointF(1.5, -0.5));
            }
            p.setPen(QPen(hover ? StyleHelper::panelTextColor() : QColor(180, 180, 180), 1.2, Qt::SolidLine, Qt::FlatCap));
            p.setRenderHint(QPainter::Antialiasing, true);
            p.drawLine(cancelVisualRect.topLeft() + QPointF(4.0, 2.0), cancelVisualRect.bottomRight() + QPointF(-3.0, -2.0));
            p.drawLine(cancelVisualRect.bottomLeft() + QPointF(4.0, -2.0), cancelVisualRect.topRight() + QPointF(-3.0, 2.0));
        }
    }
}
