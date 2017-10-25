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

#include "highlightscrollbar.h"

#include <utils/asconst.h>
#include <utils/qtcassert.h>

#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>

using namespace Utils;
namespace Core {

class HighlightScrollBarOverlay : public QWidget
{
public:
    HighlightScrollBarOverlay(HighlightScrollBar *scrollBar)
        : QWidget(scrollBar)
        , m_visibleRange(0.0)
        , m_offset(0.0)
        , m_cacheUpdateScheduled(false)
        , m_scrollBar(scrollBar)
    {}

    void scheduleUpdate();
    void updateCache();
    void adjustPosition();

    float m_visibleRange;
    float m_offset;
    QHash<Id, QVector<Highlight> > m_highlights;

    bool m_cacheUpdateScheduled;
    QMap<int, Highlight> m_cache;

protected:
    void paintEvent(QPaintEvent *paintEvent) override;

private:
    HighlightScrollBar *m_scrollBar;
};

HighlightScrollBar::HighlightScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
    , m_widget(parent)
    , m_overlay(new HighlightScrollBarOverlay(this))
{
    connect(m_overlay, &HighlightScrollBarOverlay::destroyed,
            this, &HighlightScrollBar::overlayDestroyed);
    // valueChanged(0) flashes transient scroll bars, which is needed
    // for a correct initialization.
    emit valueChanged(0);
}

HighlightScrollBar::~HighlightScrollBar()
{
    if (!m_overlay || m_overlay->parent() == this)
        return;

    delete m_overlay;
}

void HighlightScrollBar::setVisibleRange(float visibleRange)
{
    if (!m_overlay)
        return;
    m_overlay->m_visibleRange = visibleRange;
}

void HighlightScrollBar::setRangeOffset(float offset)
{
    if (!m_overlay)
        return;
    m_overlay->m_offset = offset;
}

QRect HighlightScrollBar::overlayRect()
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    return style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
}

void HighlightScrollBar::overlayDestroyed()
{
    m_overlay = 0;
}

void HighlightScrollBar::addHighlight(Highlight highlight)
{
    if (!m_overlay)
        return;
    m_overlay->m_highlights[highlight.category] << highlight;
    m_overlay->scheduleUpdate();
}

void HighlightScrollBar::removeHighlights(Id category)
{
    if (!m_overlay)
        return;
    m_overlay->m_highlights.remove(category);
    m_overlay->scheduleUpdate();
}

void HighlightScrollBar::removeAllHighlights()
{
    if (!m_overlay)
        return;
    m_overlay->m_highlights.clear();
    m_overlay->scheduleUpdate();
}

bool HighlightScrollBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_widget && m_overlay && m_widget == m_overlay->parent() &&
            (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const int width = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
        m_overlay->move(m_widget->width() - width, 0);
        resize(width, m_widget->height());
    }
    return false;
}

void HighlightScrollBar::resizeEvent(QResizeEvent *event)
{
    if (!m_overlay)
        return;
    QScrollBar::resizeEvent(event);
    m_overlay->resize(size());
}

void HighlightScrollBar::moveEvent(QMoveEvent *event)
{
    if (!m_overlay)
        return;
    QScrollBar::moveEvent(event);
    m_overlay->adjustPosition();
}

void HighlightScrollBar::showEvent(QShowEvent *event)
{
    if (!m_overlay)
        return;
    QScrollBar::showEvent(event);
    if (parentWidget() != this) {
        m_widget->removeEventFilter(this);
        m_overlay->setParent(this);
        m_overlay->adjustPosition();
        m_overlay->show();
    }
}

void HighlightScrollBar::hideEvent(QHideEvent *event)
{
    if (!m_overlay)
        return;
    QScrollBar::hideEvent(event);
    if (parentWidget() != m_widget) {
        m_widget->installEventFilter(this);
        m_overlay->setParent(m_widget);
        m_overlay->adjustPosition();
        m_overlay->show();
    }
}

void HighlightScrollBar::changeEvent(QEvent *event)
{
    // Workaround for QTBUG-45579
    if (event->type() == QEvent::ParentChange)
        setStyle(style());
}

void HighlightScrollBarOverlay::scheduleUpdate()
{
    if (m_cacheUpdateScheduled)
        return;

    m_cacheUpdateScheduled = true;
    QTimer::singleShot(0, this, static_cast<void (QWidget::*)()>(&QWidget::update));
}

void HighlightScrollBarOverlay::updateCache()
{
    if (!m_cacheUpdateScheduled)
        return;

    m_cache.clear();
    const QList<Id> &categories = m_highlights.keys();
    for (const Id &category : categories) {
        for (const Highlight &highlight : Utils::asConst(m_highlights[category])) {
            Highlight oldHighlight = m_cache[highlight.position];
            if (highlight.priority > oldHighlight.priority)
                m_cache[highlight.position] = highlight;
        }
    }
    m_cacheUpdateScheduled = false;
}

void HighlightScrollBarOverlay::adjustPosition()
{
    move(parentWidget()->mapFromGlobal(m_scrollBar->mapToGlobal(m_scrollBar->pos())));
}

void HighlightScrollBarOverlay::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    updateCache();

    if (m_cache.isEmpty())
        return;

    const QRect &rect = m_scrollBar->overlayRect();

    Utils::Theme::Color previousColor = Utils::Theme::TextColorNormal;
    Highlight::Priority previousPriority = Highlight::LowPriority;
    QRect *previousRect = 0;

    const int scrollbarRange = m_scrollBar->maximum() + m_scrollBar->pageStep();
    const int range = qMax(m_visibleRange, float(scrollbarRange));
    const int horizontalMargin = 3;
    const int resultWidth = rect.width() - 2 * horizontalMargin + 1;
    const int resultHeight = qMin(int(rect.height() / range) + 1, 4);
    const int offset = rect.height() / range * m_offset;
    const int verticalMargin = ((rect.height() / range) - resultHeight) / 2;
    int previousBottom = -1;

    QHash<Utils::Theme::Color, QVector<QRect> > highlights;
    for (Highlight currentHighlight : Utils::asConst(m_cache)) {
        // Calculate top and bottom
        int top = rect.top() + offset + verticalMargin
                + float(currentHighlight.position) / range * rect.height();
        const int bottom = top + resultHeight;

        if (previousRect && previousColor == currentHighlight.color && previousBottom + 1 >= top) {
            // If the previous highlight has the same color and is directly prior to this highlight
            // we just extend the previous highlight.
            previousRect->setBottom(bottom - 1);

        } else { // create a new highlight
            if (previousRect && previousBottom >= top) {
                // make sure that highlights with higher priority are drawn on top of other highlights
                // when rectangles are overlapping

                if (previousPriority > currentHighlight.priority) {
                    // Moving the top of the current highlight when the previous
                    // highlight has a higher priority
                    top = previousBottom + 1;
                    if (top == bottom) // if this would result in an empty highlight just skip
                        continue;
                } else {
                    previousRect->setBottom(top - 1); // move the end of the last highlight
                    if (previousRect->height() == 0) // if the result is an empty rect, remove it.
                        highlights[previousColor].removeLast();
                }
            }
            highlights[currentHighlight.color] << QRect(rect.left() + horizontalMargin, top,
                                                        resultWidth, bottom - top);
            previousRect = &highlights[currentHighlight.color].last();
            previousColor = currentHighlight.color;
            previousPriority = currentHighlight.priority;
        }
        previousBottom = previousRect->bottom();
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const auto highlightEnd = highlights.cend();
    for (auto highlightIt = highlights.cbegin(); highlightIt != highlightEnd; ++highlightIt) {
        const QColor &color = creatorTheme()->color(highlightIt.key());
        for (int i = 0, total = highlightIt.value().size(); i < total; ++i) {
            const QRect rect = highlightIt.value().at(i);
            painter.fillRect(rect, color);
        }
    }
}

Highlight::Highlight(Id category_, int position_,
                     Theme::Color color_, Highlight::Priority priority_)
    : category(category_)
    , position(position_)
    , color(color_)
    , priority(priority_)
{
}

} // namespace Core
