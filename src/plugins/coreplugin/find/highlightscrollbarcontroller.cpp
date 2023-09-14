// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlightscrollbarcontroller.h"

#include <QAbstractScrollArea>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>

using namespace Utils;

namespace Core {

/*!
    \class Core::Highlight
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::HighlightScrollBarController
    \inmodule QtCreator
    \internal
*/

class HighlightScrollBarOverlay : public QWidget
{
public:
    HighlightScrollBarOverlay(HighlightScrollBarController *scrollBarController)
        : QWidget(scrollBarController->scrollArea())
        , m_highlightController(scrollBarController)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        scrollBar()->parentWidget()->installEventFilter(this);
        doResize();
        doMove();
        show();
    }

    void doResize()
    {
        resize(scrollBar()->size());
    }

    void doMove()
    {
        move(parentWidget()->mapFromGlobal(scrollBar()->mapToGlobal(scrollBar()->pos())));
    }

    void scheduleUpdate();

protected:
    void paintEvent(QPaintEvent *paintEvent) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void drawHighlights(QPainter *painter,
                        int docStart,
                        int docSize,
                        double docSizeToHandleSizeRatio,
                        int handleOffset,
                        const QRect &viewport);
    void updateCache();
    QRect overlayRect() const;
    QRect handleRect() const;

    // line start to line end
    QMap<Highlight::Priority, QMap<Utils::Theme::Color, QMap<int, int>>> m_highlightCache;

    inline QScrollBar *scrollBar() const { return m_highlightController->scrollBar(); }
    HighlightScrollBarController *m_highlightController;
    bool m_isCacheUpdateScheduled = true;
};

void HighlightScrollBarOverlay::scheduleUpdate()
{
    if (m_isCacheUpdateScheduled)
        return;

    m_isCacheUpdateScheduled = true;
    QMetaObject::invokeMethod(this, QOverload<>::of(&QWidget::update), Qt::QueuedConnection);
}

void HighlightScrollBarOverlay::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    updateCache();

    if (m_highlightCache.isEmpty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRect &gRect = overlayRect();
    const QRect &hRect = handleRect();

    constexpr int marginX = 3;
    constexpr int marginH = -2 * marginX + 1;
    const QRect aboveHandleRect = QRect(gRect.x() + marginX,
                                        gRect.y(),
                                        gRect.width() + marginH,
                                        hRect.y() - gRect.y());
    const QRect handleRect      = QRect(gRect.x() + marginX,
                                        hRect.y(),
                                        gRect.width() + marginH,
                                        hRect.height());
    const QRect belowHandleRect = QRect(gRect.x() + marginX,
                                        hRect.y() + hRect.height(),
                                        gRect.width() + marginH,
                                        gRect.height() - hRect.height() + gRect.y() - hRect.y());

    const int aboveValue = scrollBar()->value();
    const int belowValue = scrollBar()->maximum() - scrollBar()->value();
    const int sizeDocAbove = int(aboveValue * m_highlightController->lineHeight());
    const int sizeDocBelow = int(belowValue * m_highlightController->lineHeight());
    const int sizeDocVisible = int(m_highlightController->visibleRange());

    const int scrollBarBackgroundHeight = aboveHandleRect.height() + belowHandleRect.height();
    const int sizeDocInvisible = sizeDocAbove + sizeDocBelow;
    const double backgroundRatio = sizeDocInvisible
            ? ((double)scrollBarBackgroundHeight / sizeDocInvisible) : 0;


    if (aboveValue) {
        drawHighlights(&painter,
                       0,
                       sizeDocAbove,
                       backgroundRatio,
                       0,
                       aboveHandleRect);
    }

    if (belowValue) {
        // This is the hypothetical handle height if the handle would
        // be stretched using the background ratio.
        const double handleVirtualHeight = sizeDocVisible * backgroundRatio;
        // Skip the doc above and visible part.
        const int offset = qRound(aboveHandleRect.height() + handleVirtualHeight);

        drawHighlights(&painter,
                       sizeDocAbove + sizeDocVisible,
                       sizeDocBelow,
                       backgroundRatio,
                       offset,
                       belowHandleRect);
    }

    const double handleRatio = sizeDocVisible
            ? ((double)handleRect.height() / sizeDocVisible) : 0;

    // This is the hypothetical handle position if the background would
    // be stretched using the handle ratio.
    const double aboveVirtualHeight = sizeDocAbove * handleRatio;

    // This is the accurate handle position (double)
    const double accurateHandlePos = sizeDocAbove * backgroundRatio;
    // The correction between handle position (int) and accurate position (double)
    const double correction = aboveHandleRect.height() - accurateHandlePos;
    // Skip the doc above and apply correction
    const int offset = qRound(aboveVirtualHeight + correction);

    drawHighlights(&painter,
                   sizeDocAbove,
                   sizeDocVisible,
                   handleRatio,
                   offset,
                   handleRect);
}

void HighlightScrollBarOverlay::drawHighlights(QPainter *painter,
                                          int docStart,
                                          int docSize,
                                          double docSizeToHandleSizeRatio,
                                          int handleOffset,
                                          const QRect &viewport)
{
    if (docSize <= 0)
        return;

    painter->save();
    painter->setClipRect(viewport);

    const double lineHeight = m_highlightController->lineHeight();

    for (const QMap<Utils::Theme::Color, QMap<int, int>> &colors : std::as_const(m_highlightCache)) {
        const auto itColorEnd = colors.constEnd();
        for (auto itColor = colors.constBegin(); itColor != itColorEnd; ++itColor) {
            const QColor &color = creatorTheme()->color(itColor.key());
            const QMap<int, int> &positions = itColor.value();
            const auto itPosEnd = positions.constEnd();
            const auto firstPos = int(docStart / lineHeight);
            auto itPos = positions.upperBound(firstPos);
            if (itPos != positions.constBegin())
                --itPos;
            while (itPos != itPosEnd) {
                const double posStart = itPos.key() * lineHeight;
                const double posEnd = (itPos.value() + 1) * lineHeight;
                if (posEnd < docStart) {
                    ++itPos;
                    continue;
                }
                if (posStart > docStart + docSize)
                    break;

                const int height = qMax(qRound((posEnd - posStart) * docSizeToHandleSizeRatio), 1);
                const int top = qRound(posStart * docSizeToHandleSizeRatio) - handleOffset + viewport.y();

                const QRect rect(viewport.left(), top, viewport.width(), height);
                painter->fillRect(rect, color);
                ++itPos;
            }
        }
    }
    painter->restore();
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
    case QEvent::Show:
        show();
        break;
    case QEvent::Hide:
        hide();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}

static void insertPosition(QMap<int, int> *map, int position)
{
    auto itNext = map->upperBound(position);

    bool gluedWithPrev = false;
    if (itNext != map->begin()) {
        auto itPrev = std::prev(itNext);
        const int keyStart = itPrev.key();
        const int keyEnd = itPrev.value();
        if (position >= keyStart && position <= keyEnd)
            return; // pos is already included

        if (keyEnd + 1 == position) {
            // glue with prev
            (*itPrev)++;
            gluedWithPrev = true;
        }
    }

    if (itNext != map->end() && itNext.key() == position + 1) {
        const int keyEnd = itNext.value();
        itNext = map->erase(itNext);
        if (gluedWithPrev) {
            // glue with prev and next
            auto itPrev = std::prev(itNext);
            *itPrev = keyEnd;
        } else {
            // glue with next
            itNext = map->insert(itNext, position, keyEnd);
        }
        return; // glued
    }

    if (gluedWithPrev)
        return; // glued

    map->insert(position, position);
}

void HighlightScrollBarOverlay::updateCache()
{
    if (!m_isCacheUpdateScheduled)
        return;

    m_highlightCache.clear();

    const QHash<Id, QVector<Highlight>> highlightsForId = m_highlightController->highlights();
    for (const QVector<Highlight> &highlights : highlightsForId) {
        for (const auto &highlight : highlights) {
            auto &highlightMap = m_highlightCache[highlight.priority][highlight.color];
            insertPosition(&highlightMap, highlight.position);
        }
    }

    m_isCacheUpdateScheduled = false;
}

QRect HighlightScrollBarOverlay::overlayRect() const
{
    QStyleOptionSlider opt = qt_qscrollbarStyleOption(scrollBar());
    return scrollBar()->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, scrollBar());
}

QRect HighlightScrollBarOverlay::handleRect() const
{
    QStyleOptionSlider opt = qt_qscrollbarStyleOption(scrollBar());
    return scrollBar()->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, scrollBar());
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

double HighlightScrollBarController::lineHeight() const
{
    return ceil(m_lineHeight);
}

void HighlightScrollBarController::setLineHeight(double lineHeight)
{
    m_lineHeight = lineHeight;
}

double HighlightScrollBarController::visibleRange() const
{
    return m_visibleRange;
}

void HighlightScrollBarController::setVisibleRange(double visibleRange)
{
    m_visibleRange = visibleRange;
}

double HighlightScrollBarController::margin() const
{
    return m_margin;
}

void HighlightScrollBarController::setMargin(double margin)
{
    m_margin = margin;
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
