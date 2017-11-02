/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "highlightscrollbarcontroller.h"

#include <utils/asconst.h>

#include <QAbstractScrollArea>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>

using namespace Utils;
namespace Core {

class HighlightScrollBarOverlay : public QWidget
{
public:
    HighlightScrollBarOverlay(HighlightScrollBarController *scrollBarController)
        : QWidget(scrollBarController->scrollArea())
        , m_scrollBar(scrollBarController->scrollBar())
        , m_highlightController(scrollBarController)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_scrollBar->parentWidget()->installEventFilter(this);
        doResize();
        doMove();
        show();
    }

    void doResize()
    {
        resize(m_scrollBar->size());
    }

    void doMove()
    {
        move(parentWidget()->mapFromGlobal(m_scrollBar->mapToGlobal(m_scrollBar->pos())));
    }

    void scheduleUpdate();

protected:
    void paintEvent(QPaintEvent *paintEvent) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void updateCache();
    QRect overlayRect() const;

    bool m_cacheUpdateScheduled = true;
    QMap<int, Highlight> m_cache;

    QScrollBar *m_scrollBar;
    HighlightScrollBarController *m_highlightController;
};

void HighlightScrollBarOverlay::scheduleUpdate()
{
    if (m_cacheUpdateScheduled)
        return;

    m_cacheUpdateScheduled = true;
    QTimer::singleShot(0, this, static_cast<void (QWidget::*)()>(&QWidget::update));
}

void HighlightScrollBarOverlay::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    updateCache();

    if (m_cache.isEmpty())
        return;

    const QRect &rect = overlayRect();

    Utils::Theme::Color previousColor = Utils::Theme::TextColorNormal;
    Highlight::Priority previousPriority = Highlight::LowPriority;
    QRect *previousRect = nullptr;

    const int scrollbarRange = m_scrollBar->maximum() + m_scrollBar->pageStep();
    const int range = qMax(m_highlightController->visibleRange(), float(scrollbarRange));
    const int horizontalMargin = 3;
    const int resultWidth = rect.width() - 2 * horizontalMargin + 1;
    const int resultHeight = qMin(int(rect.height() / range) + 1, 4);
    const int offset = rect.height() / range * m_highlightController->rangeOffset();
    const int verticalMargin = ((rect.height() / range) - resultHeight) / 2;
    int previousBottom = -1;

    QHash<Utils::Theme::Color, QVector<QRect> > highlights;
    for (const Highlight &currentHighlight : Utils::asConst(m_cache)) {
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
        for (const QRect &rect : highlightIt.value())
            painter.fillRect(rect, color);
    }
}

bool HighlightScrollBarOverlay::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Move:
        doMove();
        break;
    case QEvent::Resize:
        doResize();
        break;
    case QEvent::ZOrderChange:
        raise();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}

void HighlightScrollBarOverlay::updateCache()
{
    if (!m_cacheUpdateScheduled)
        return;

    m_cache.clear();
    const QHash<Id, QVector<Highlight>> highlights = m_highlightController->highlights();
    const QList<Id> &categories = highlights.keys();
    for (const Id &category : categories) {
        for (const Highlight &highlight : highlights.value(category)) {
            const Highlight oldHighlight = m_cache.value(highlight.position);
            if (highlight.priority > oldHighlight.priority)
                m_cache[highlight.position] = highlight;
        }
    }
    m_cacheUpdateScheduled = false;
}

QRect HighlightScrollBarOverlay::overlayRect() const
{
    QStyleOptionSlider opt = qt_qscrollbarStyleOption(m_scrollBar);
    return m_scrollBar->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, m_scrollBar);
}

/////////////

Highlight::Highlight(Id category_, int position_,
                     Theme::Color color_, Highlight::Priority priority_)
    : category(category_)
    , position(position_)
    , color(color_)
    , priority(priority_)
{
}

/////////////

HighlightScrollBarController::~HighlightScrollBarController()
{
    if (m_overlay)
        delete m_overlay;
}

QScrollBar *HighlightScrollBarController::scrollBar() const
{
    if (m_scrollArea)
        return m_scrollArea->verticalScrollBar();

    return nullptr;
}

QAbstractScrollArea *HighlightScrollBarController::scrollArea() const
{
    return m_scrollArea;
}

void HighlightScrollBarController::setScrollArea(QAbstractScrollArea *scrollArea)
{
    if (m_scrollArea == scrollArea)
        return;

    if (m_overlay) {
        delete m_overlay;
        m_overlay = nullptr;
    }

    m_scrollArea = scrollArea;

    if (m_scrollArea) {
        m_overlay = new HighlightScrollBarOverlay(this);
        m_overlay->scheduleUpdate();
    }
}

float HighlightScrollBarController::visibleRange() const
{
    return m_visibleRange;
}

void HighlightScrollBarController::setVisibleRange(float visibleRange)
{
    m_visibleRange = visibleRange;
}

float HighlightScrollBarController::rangeOffset() const
{
    return m_rangeOffset;
}

void HighlightScrollBarController::setRangeOffset(float offset)
{
    m_rangeOffset = offset;
}

QHash<Id, QVector<Highlight>> HighlightScrollBarController::highlights() const
{
    return m_highlights;
}

void HighlightScrollBarController::addHighlight(Highlight highlight)
{
    if (!m_overlay)
        return;

    m_highlights[highlight.category] << highlight;
    m_overlay->scheduleUpdate();
}

void HighlightScrollBarController::removeHighlights(Id category)
{
    if (!m_overlay)
        return;

    m_highlights.remove(category);
    m_overlay->scheduleUpdate();
}

void HighlightScrollBarController::removeAllHighlights()
{
    if (!m_overlay)
        return;

    m_highlights.clear();
    m_overlay->scheduleUpdate();
}

} // namespace Core
