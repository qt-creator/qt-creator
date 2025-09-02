// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalview.h"
#include "glyphcache.h"
#include "terminalsurface.h"

#include <vterm.h>

#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QDesktopServices>
#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QMenu>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRawFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextItem>
#include <QTextLayout>
#include <QToolTip>

Q_LOGGING_CATEGORY(terminalLog, "qtc.terminal", QtWarningMsg)
Q_LOGGING_CATEGORY(selectionLog, "qtc.terminal.selection", QtWarningMsg)
Q_LOGGING_CATEGORY(paintLog, "qtc.terminal.paint", QtWarningMsg)

namespace TerminalSolution {

using namespace std::chrono;
using namespace std::chrono_literals;

// Minimum time between two refreshes. (30fps)
static constexpr milliseconds minRefreshInterval = 33ms;

class TerminalViewPrivate
{
public:
    TerminalViewPrivate()
    {
        m_cursorBlinkTimer.setInterval(750ms);
        m_cursorBlinkTimer.setSingleShot(false);

        m_flushDelayTimer.setSingleShot(true);
        m_flushDelayTimer.setInterval(minRefreshInterval);

        m_updateTimer.setSingleShot(true);
        m_updateTimer.setInterval(minRefreshInterval);

        m_scrollTimer.setSingleShot(false);
        m_scrollTimer.setInterval(500ms);
    }

    std::optional<TerminalView::Selection> m_selection;
    std::unique_ptr<TerminalSurface> m_surface;

    QSizeF m_cellSize;

    bool m_ignoreScroll{false};

    QString m_preEditString;

    std::optional<TerminalView::LinkSelection> m_linkSelection;

    struct
    {
        QPoint start;
        QPoint end;
    } m_activeMouseSelect;

    QTimer m_flushDelayTimer;

    QTimer m_updateTimer;
    std::optional<QRegion> m_updateRegion;
    QDeadlineTimer m_sinceLastPaint;

    QTimer m_scrollTimer;
    int m_scrollDirection{0};

    std::array<QColor, 20> m_currentColors;

    system_clock::time_point m_lastFlush{system_clock::now()};
    system_clock::time_point m_lastDoubleClick{system_clock::now()};
    bool m_selectLineMode{false};
    Cursor m_cursor;
    QTimer m_cursorBlinkTimer;
    bool m_cursorBlinkState{true};
    bool m_allowBlinkingCursor{true};
    bool m_allowMouseTracking{true};
    bool m_passwordModeActive{false};

    SurfaceIntegration *m_surfaceIntegration{nullptr};
};

QString defaultFontFamily()
{
#ifdef Q_OS_DARWIN
    return QLatin1String("Menlo");
#elif defined(Q_OS_WIN)
    return QLatin1String("Consolas");
#else
    return QLatin1String("Monospace");
#endif
}

int defaultFontSize()
{
#ifdef Q_OS_DARWIN
        return 12;
#elif defined(Q_OS_WIN)
    return 10;
#else
    return 9;
#endif
}

TerminalView::TerminalView(QWidget *parent)
    : QAbstractScrollArea(parent)
    , d(std::make_unique<TerminalViewPrivate>())
{
    setupSurface();
    setFont(QFont(defaultFontFamily(), defaultFontSize()));

    connect(&d->m_cursorBlinkTimer, &QTimer::timeout, this, [this] {
        if (hasFocus())
            d->m_cursorBlinkState = !d->m_cursorBlinkState;
        else
            d->m_cursorBlinkState = true;
        updateViewportRect(gridToViewport(QRect{d->m_cursor.position, d->m_cursor.position}));
    });

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_MouseTracking);
    setAcceptDrops(true);

    setCursor(Qt::IBeamCursor);

    setViewportMargins(1, 1, 1, 1);

    setFocus();
    setFocusPolicy(Qt::StrongFocus);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(&d->m_flushDelayTimer, &QTimer::timeout, this, [this] { flushVTerm(true); });
    connect(&d->m_updateTimer, &QTimer::timeout, this, &TerminalView::scheduleViewportUpdate);

    connect(&d->m_scrollTimer, &QTimer::timeout, this, [this] {
        if (d->m_scrollDirection < 0)
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        else if (d->m_scrollDirection > 0)
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    });
}

TerminalView::~TerminalView() = default;

void TerminalView::setSurfaceIntegration(SurfaceIntegration *surfaceIntegration)
{
    d->m_surfaceIntegration = surfaceIntegration;
    if (d->m_surface)
        d->m_surface->setSurfaceIntegration(d->m_surfaceIntegration);
}

TerminalSurface *TerminalView::surface() const
{
    return d->m_surface.get();
}

void TerminalView::setupSurface()
{
    d->m_surface = std::make_unique<TerminalSurface>(QSize{80, 60});
    connect(d->m_surface.get(), &TerminalSurface::cleared, this, &TerminalView::cleared);

    if (d->m_surfaceIntegration)
        d->m_surface->setSurfaceIntegration(d->m_surfaceIntegration);

    d->m_surface->setWriteToPty([this](const QByteArray &data) { return writeToPty(data); });

    connect(d->m_surface.get(), &TerminalSurface::fullSizeChanged, this, [this] {
        updateScrollBars();
    });
    connect(d->m_surface.get(), &TerminalSurface::invalidated, this, [this](const QRect &rect) {
        setSelection(std::nullopt);
        updateViewportRect(gridToViewport(rect));
        if (verticalScrollBar()->value() == verticalScrollBar()->maximum())
            verticalScrollBar()->setValue(d->m_surface->fullSize().height());
    });
    connect(
        d->m_surface.get(),
        &TerminalSurface::cursorChanged,
        this,
        [this](const Cursor &oldCursor, const Cursor &newCursor) {
            int startX = oldCursor.position.x();
            int endX = newCursor.position.x();

            if (startX > endX)
                std::swap(startX, endX);

            int startY = oldCursor.position.y();
            int endY = newCursor.position.y();
            if (startY > endY)
                std::swap(startY, endY);

            d->m_cursor = newCursor;

            updateViewportRect(gridToViewport(QRect{QPoint{startX, startY}, QPoint{endX, endY}}));
            configBlinkTimer();
        });
    connect(d->m_surface.get(), &TerminalSurface::altscreenChanged, this, [this] {
        updateScrollBars();
        if (!setSelection(std::nullopt))
            updateViewport();
    });
    connect(d->m_surface.get(), &TerminalSurface::unscroll, this, [this] {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    });

    surfaceChanged();
    updateScrollBars();
}

void TerminalView::setAllowBlinkingCursor(bool allow)
{
    d->m_allowBlinkingCursor = allow;
}
bool TerminalView::allowBlinkingCursor() const
{
    return d->m_allowBlinkingCursor;
}

void TerminalView::configBlinkTimer()
{
    bool shouldRun = d->m_cursor.visible && d->m_cursor.blink && hasFocus()
                     && d->m_allowBlinkingCursor;
    if (shouldRun != d->m_cursorBlinkTimer.isActive()) {
        if (shouldRun)
            d->m_cursorBlinkTimer.start();
        else
            d->m_cursorBlinkTimer.stop();
    }
}

QColor TerminalView::toQColor(std::variant<int, QColor> color) const
{
    if (std::holds_alternative<int>(color)) {
        int idx = std::get<int>(color);
        if (idx >= 0 && idx < 18)
            return d->m_currentColors[idx];

        return d->m_currentColors[(int) WidgetColorIdx::Background];
    }
    return std::get<QColor>(color);
}

void TerminalView::setColors(const std::array<QColor, 20> &newColors)
{
    if (d->m_currentColors == newColors)
        return;

    d->m_currentColors = newColors;

    updateViewport();
    update();
}

void TerminalView::setPasswordMode(bool passwordMode)
{
    if (passwordMode != d->m_passwordModeActive) {
        d->m_passwordModeActive = passwordMode;
        updateViewport();
    }
}

void TerminalView::enableMouseTracking(bool enable)
{
    d->m_allowMouseTracking = enable;
}

void TerminalView::setFont(const QFont &font)
{
    QAbstractScrollArea::setFont(font);

    QFontMetricsF qfm{font};
    qCInfo(terminalLog) << font.family() << font.pointSize() << qfm.averageCharWidth()
                        << qfm.maxWidth() << viewport()->size();

    d->m_cellSize = {qfm.averageCharWidth(), (double) qCeil(qfm.height())};

    QAbstractScrollArea::setFont(font);

    applySizeChange();
}

void TerminalView::copyToClipboard()
{
    if (!d->m_selection.has_value())
        return;

    QString text = textFromSelection();

    qCDebug(selectionLog) << "Copied to clipboard: " << text;

    setClipboard(text);

    clearSelection();
}

void TerminalView::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QString clipboardText = clipboard->text(QClipboard::Clipboard);

    if (clipboardText.isEmpty())
        return;

    d->m_surface->pasteFromClipboard(clipboardText);
}

void TerminalView::copyLinkToClipboard()
{
    if (d->m_linkSelection)
        setClipboard(d->m_linkSelection->link.text);
}

std::optional<TerminalView::Selection> TerminalView::selection() const
{
    return d->m_selection;
}

void TerminalView::clearSelection()
{
    setSelection(std::nullopt);
    //d->m_surface->sendKey(Qt::Key_Escape);
}

void TerminalView::selectAll()
{
    setSelection(Selection{0, d->m_surface->fullSize().width() * d->m_surface->fullSize().height()});
}

void TerminalView::zoomIn()
{
    QFont f = font();
    f.setPointSize(f.pointSize() + 1);
    setFont(f);
}

void TerminalView::zoomOut()
{
    QFont f = font();
    f.setPointSize(qMax(f.pointSize() - 1, 1));
    setFont(f);
}

void TerminalView::moveCursorWordLeft()
{
    writeToPty("\x1b\x62");
}

void TerminalView::moveCursorWordRight()
{
    writeToPty("\x1b\x66");
}

void TerminalView::clearContents()
{
    d->m_surface->clearAll();
}

void TerminalView::writeToTerminal(const QByteArray &data, bool forceFlush)
{
    d->m_surface->dataFromPty(data);
    flushVTerm(forceFlush);
}

void TerminalView::flushVTerm(bool force)
{
    const system_clock::time_point now = system_clock::now();
    const milliseconds timeSinceLastFlush = duration_cast<milliseconds>(now - d->m_lastFlush);

    const bool shouldFlushImmediately = timeSinceLastFlush > minRefreshInterval;
    if (force || shouldFlushImmediately) {
        if (d->m_flushDelayTimer.isActive())
            d->m_flushDelayTimer.stop();

        d->m_lastFlush = now;
        d->m_surface->flush();
        return;
    }

    if (!d->m_flushDelayTimer.isActive()) {
        const milliseconds timeToNextFlush = (minRefreshInterval - timeSinceLastFlush);
        d->m_flushDelayTimer.start(timeToNextFlush.count());
    }
}

QString TerminalView::textFromSelection() const
{
    if (!d->m_selection)
        return {};

    if (d->m_selection->start == d->m_selection->end)
        return {};

    CellIterator it = d->m_surface->iteratorAt(d->m_selection->start);
    CellIterator end = d->m_surface->iteratorAt(d->m_selection->end);

    if (it.position() > end.position()) {
        qCWarning(selectionLog) << "Invalid selection: start >= end";
        return {};
    }

    std::u32string s;
    bool previousWasZero = false;
    for (; it != end; ++it) {
        if (it.gridPos().x() == 0 && !s.empty() && previousWasZero)
            s += U'\n';

        if (*it != 0) {
            previousWasZero = false;
            s += *it;
        } else {
            previousWasZero = true;
        }
    }

    return QString::fromUcs4(s.data(), static_cast<int>(s.size()));
}

bool TerminalView::setSelection(const std::optional<Selection> &selection, bool scroll)
{
    qCDebug(selectionLog) << "setSelection" << selection.has_value();
    if (selection.has_value())
        qCDebug(selectionLog) << "start:" << selection->start << "end:" << selection->end
                              << "final:" << selection->final;

    if (selectionLog().isDebugEnabled())
        updateViewport();

    if (selection == d->m_selection)
        return false;

    d->m_selection = selection;
    selectionChanged(d->m_selection);

    if (d->m_selection && d->m_selection->final && scroll) {
        QPoint start = d->m_surface->posToGrid(d->m_selection->start);
        QPoint end = d->m_surface->posToGrid(d->m_selection->end);
        QRect viewRect = gridToViewport(QRect{start, end});
        if (viewRect.y() >= viewport()->height() || viewRect.y() < 0) {
            // Selection is outside of the viewport, scroll to it.
            verticalScrollBar()->setValue(start.y());
        }
    }

    if (!selectionLog().isDebugEnabled())
        updateViewport();

    return true;
}

void TerminalView::restart()
{
    setupSurface();
    applySizeChange();
}

QPoint TerminalView::viewportToGlobal(QPoint p) const
{
    int y = p.y() - topMargin();
    const double offset = verticalScrollBar()->value() * d->m_cellSize.height();
    y += offset;

    return {p.x(), y};
}

QPoint TerminalView::globalToViewport(QPoint p) const
{
    int y = p.y() + topMargin();
    const double offset = verticalScrollBar()->value() * d->m_cellSize.height();
    y -= offset;

    return {p.x(), y};
}

QPoint TerminalView::globalToGrid(QPointF p) const
{
    return QPoint(p.x() / d->m_cellSize.width(), p.y() / d->m_cellSize.height());
}

QPointF TerminalView::gridToGlobal(QPoint p, bool bottom, bool right) const
{
    QPointF result = QPointF(p.x() * d->m_cellSize.width(), p.y() * d->m_cellSize.height());
    if (bottom || right)
        result += {right ? d->m_cellSize.width() : 0, bottom ? d->m_cellSize.height() : 0};
    return result;
}

qreal TerminalView::topMargin() const
{
    return viewport()->size().height()
           - (d->m_surface->liveSize().height() * d->m_cellSize.height());
}

static QPixmap generateWavyPixmap(qreal maxRadius, const QPen &pen)
{
    const qreal radiusBase = qMax(qreal(1), maxRadius);
    const qreal pWidth = pen.widthF();

    const QString key = QLatin1String("WaveUnderline-") % pen.color().name()
                        % QString::number(int(radiusBase), 16) % QString::number(int(pWidth), 16);

    QPixmap pixmap;
    if (QPixmapCache::find(key, &pixmap))
        return pixmap;

    const qreal halfPeriod = qMax(qreal(2), qreal(radiusBase * 1.61803399)); // the golden ratio
    const int width = qCeil(100 / (2 * halfPeriod)) * (2 * halfPeriod);
    const qreal radius = qFloor(radiusBase * 2) / 2.;

    QPainterPath path;

    qreal xs = 0;
    qreal ys = radius;

    while (xs < width) {
        xs += halfPeriod;
        ys = -ys;
        path.quadTo(xs - halfPeriod / 2, ys, xs, 0);
    }

    pixmap = QPixmap(width, radius * 2);
    pixmap.fill(Qt::transparent);
    {
        QPen wavePen = pen;
        wavePen.setCapStyle(Qt::SquareCap);

        // This is to protect against making the line too fat, as happens on macOS
        // due to it having a rather thick width for the regular underline.
        const qreal maxPenWidth = .8 * radius;
        if (wavePen.widthF() > maxPenWidth)
            wavePen.setWidthF(maxPenWidth);

        QPainter imgPainter(&pixmap);
        imgPainter.setPen(wavePen);
        imgPainter.setRenderHint(QPainter::Antialiasing);
        imgPainter.translate(0, radius);
        imgPainter.drawPath(path);
    }

    QPixmapCache::insert(key, pixmap);

    return pixmap;
}

// Copied from qpainter.cpp
static void drawTextItemDecoration(QPainter &painter,
                                   const QPointF &pos,
                                   QTextCharFormat::UnderlineStyle underlineStyle,
                                   QTextItem::RenderFlags flags,
                                   qreal width,
                                   const QColor &underlineColor,
                                   const QRawFont &font)
{
    if (underlineStyle == QTextCharFormat::NoUnderline
        && !(flags & (QTextItem::StrikeOut | QTextItem::Overline)))
        return;

    const QPen oldPen = painter.pen();
    const QBrush oldBrush = painter.brush();
    painter.setBrush(Qt::NoBrush);
    QPen pen = oldPen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidthF(font.lineThickness());
    pen.setCapStyle(Qt::FlatCap);

    QLineF line(qFloor(pos.x()), pos.y(), qFloor(pos.x() + width), pos.y());

    const qreal underlineOffset = font.underlinePosition();

    /*if (underlineStyle == QTextCharFormat::SpellCheckUnderline) {
        QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
        if (theme)
            underlineStyle = QTextCharFormat::UnderlineStyle(
                theme->themeHint(QPlatformTheme::SpellCheckUnderlineStyle).toInt());
        if (underlineStyle == QTextCharFormat::SpellCheckUnderline) // still not resolved
            underlineStyle = QTextCharFormat::WaveUnderline;
    }*/

    if (underlineStyle == QTextCharFormat::WaveUnderline) {
        painter.save();
        painter.translate(0, pos.y() + 1);
        qreal maxHeight = font.descent() - qreal(1);

        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);

        // Adapt wave to underlineOffset or pen width, whatever is larger, to make it work on all platforms
        const QPixmap wave = generateWavyPixmap(qMin(qMax(underlineOffset, pen.widthF()),
                                                     maxHeight / qreal(2.)),
                                                pen);
        const int descent = qFloor(maxHeight);

        painter.setBrushOrigin(painter.brushOrigin().x(), 0);
        painter.fillRect(pos.x(), 0, qCeil(width), qMin(wave.height(), descent), wave);
        painter.restore();
    } else if (underlineStyle != QTextCharFormat::NoUnderline) {
        // Deliberately ceil the offset to avoid the underline coming too close to
        // the text above it, but limit it to stay within descent.
        qreal adjustedUnderlineOffset = std::ceil(underlineOffset) + 0.5;
        if (underlineOffset <= font.descent())
            adjustedUnderlineOffset = qMin(adjustedUnderlineOffset, font.descent() - qreal(0.5));
        const qreal underlinePos = pos.y() + adjustedUnderlineOffset;
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);

        pen.setStyle((Qt::PenStyle)(underlineStyle));
        painter.setPen(pen);
        QLineF underline(line.x1(), underlinePos, line.x2(), underlinePos);
        painter.drawLine(underline);
    }

    pen.setStyle(Qt::SolidLine);
    pen.setColor(oldPen.color());

    if (flags & QTextItem::StrikeOut) {
        QLineF strikeOutLine = line;
        strikeOutLine.translate(0., -font.ascent() / 3.);
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);
        painter.setPen(pen);
        painter.drawLine(strikeOutLine);
    }

    if (flags & QTextItem::Overline) {
        QLineF overline = line;
        overline.translate(0., -font.ascent());
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);
        painter.setPen(pen);
        painter.drawLine(overline);
    }

    painter.setPen(oldPen);
    painter.setBrush(oldBrush);
}

bool TerminalView::paintFindMatches(QPainter &p,
                                    QList<SearchHit>::const_iterator &it,
                                    const QRectF &cellRect,
                                    const QPoint gridPos) const
{
    if (it == searchHits().constEnd())
        return false;

    const int pos = d->m_surface->gridToPos(gridPos);
    while (it != searchHits().constEnd()) {
        if (pos < it->start)
            return false;

        if (pos >= it->end) {
            ++it;
            continue;
        }
        break;
    }

    if (it == searchHits().constEnd())
        return false;

    p.fillRect(cellRect, d->m_currentColors[(size_t) WidgetColorIdx::FindMatch]);

    return true;
}

bool TerminalView::paintSelection(QPainter &p, const QRectF &cellRect, const QPoint gridPos) const
{
    bool isInSelection = false;
    const int pos = d->m_surface->gridToPos(gridPos);

    if (d->m_selection)
        isInSelection = pos >= d->m_selection->start && pos < d->m_selection->end;

    if (isInSelection)
        p.fillRect(cellRect, d->m_currentColors[(size_t) WidgetColorIdx::Selection]);

    return isInSelection;
}

int TerminalView::paintCell(QPainter &p,
                            const QRectF &cellRect,
                            QPoint gridPos,
                            const TerminalCell &cell,
                            QFont &f,
                            QList<SearchHit>::const_iterator &searchIt) const
{
    bool paintBackground = !paintSelection(p, cellRect, gridPos)
                           && !paintFindMatches(p, searchIt, cellRect, gridPos);

    bool isDefaultBg = std::holds_alternative<int>(cell.backgroundColor)
                       && std::get<int>(cell.backgroundColor) == 17;

    if (paintBackground && !isDefaultBg)
        p.fillRect(cellRect, toQColor(cell.backgroundColor));

    p.setPen(toQColor(cell.foregroundColor));

    f.setBold(cell.bold);
    f.setItalic(cell.italic);

    if (!cell.text.isEmpty()) {
        const auto r = GlyphCache::instance().get(f, cell.text);

        if (r) {
            const auto brSize = r->boundingRect().size();
            QPointF brOffset;
            if (brSize.width() > cellRect.size().width())
                brOffset.setX(-(brSize.width() - cellRect.size().width()) / 2.0);
            if (brSize.height() > cellRect.size().height())
                brOffset.setY(-(brSize.height() - cellRect.size().height()) / 2.0);

            QPointF finalPos = cellRect.topLeft() + brOffset;

            p.drawGlyphRun(finalPos, *r);

            bool tempLink = false;
            if (d->m_linkSelection) {
                int chPos = d->m_surface->gridToPos(gridPos);
                tempLink = chPos >= d->m_linkSelection->start && chPos < d->m_linkSelection->end;
            }
            if (cell.underlineStyle != QTextCharFormat::NoUnderline || cell.strikeOut || tempLink) {
                QTextItem::RenderFlags flags;
                //flags.setFlag(QTextItem::RenderFlag::Underline, cell.format.fontUnderline());
                flags.setFlag(QTextItem::StrikeOut, cell.strikeOut);
                finalPos.setY(finalPos.y() + r->rawFont().ascent());
                drawTextItemDecoration(p,
                                       finalPos,
                                       tempLink ? QTextCharFormat::DashUnderline
                                                : cell.underlineStyle,
                                       flags,
                                       cellRect.size().width(),
                                       {},
                                       r->rawFont());
            }
        }
    }

    return cell.width;
}

void TerminalView::paintCursor(QPainter &p) const
{
    auto cursor = d->m_surface->cursor();

    const int cursorCellWidth = d->m_surface->cellWidthAt(cursor.position.x(), cursor.position.y());

    if (!d->m_preEditString.isEmpty()) {
        cursor.shape = Cursor::Shape::Underline;
    } else if (d->m_passwordModeActive) {
        QRectF cursorRect = QRectF(gridToGlobal(cursor.position),
                                   gridToGlobal({cursor.position.x() + cursorCellWidth,
                                                 cursor.position.y()},
                                                true))
                                .toAlignedRect();

        const qreal dpr = p.device()->devicePixelRatioF();
        const QString key = QString("terminalpasswordlock-")
                            % QString::number(cursorRect.size().height())
                            % "@" % QString::number(dpr);
        QPixmap px;
        if (!QPixmapCache::find(key, &px)) {
            const QPixmap lock(":/terminal/images/passwordlock.png");
            px = lock.scaledToHeight(cursorRect.size().height() * dpr, Qt::SmoothTransformation);
            px.setDevicePixelRatio(dpr);
            QPixmapCache::insert(key, px);
        }

        p.drawPixmap(cursorRect.topLeft(), px);

        return;
    }
    const bool blinkState = !cursor.blink || d->m_cursorBlinkState || !d->m_allowBlinkingCursor
                            || !d->m_cursorBlinkTimer.isActive();

    if (cursor.visible && blinkState) {
        QRectF cursorRect = QRectF(gridToGlobal(cursor.position),
                                   gridToGlobal({cursor.position.x() + cursorCellWidth,
                                                 cursor.position.y()},
                                                true))
                                .toAlignedRect();

        cursorRect.adjust(1, 1, -1, -1);

        QPen pen(Qt::white, 0, Qt::SolidLine);
        p.setPen(pen);

        if (hasFocus()) {
            QPainter::CompositionMode oldMode = p.compositionMode();
            p.setCompositionMode(QPainter::RasterOp_NotDestination);
            switch (cursor.shape) {
            case Cursor::Shape::Block:
                p.fillRect(cursorRect, p.pen().brush());
                break;
            case Cursor::Shape::Underline:
                p.drawLine(cursorRect.bottomLeft(), cursorRect.bottomRight());
                break;
            case Cursor::Shape::LeftBar:
                p.drawLine(cursorRect.topLeft(), cursorRect.bottomLeft());
                break;
            }
            p.setCompositionMode(oldMode);
        } else {
            p.drawRect(cursorRect);
        }
    }
}

void TerminalView::paintPreedit(QPainter &p) const
{
    auto cursor = d->m_surface->cursor();
    if (!d->m_preEditString.isEmpty()) {
        QRectF rect = QRectF(gridToGlobal(cursor.position),
                             gridToGlobal({cursor.position.x(), cursor.position.y()}, true, true));

        rect.setWidth(viewport()->width() - rect.x());

        p.setPen(toQColor((int) WidgetColorIdx::Foreground));
        QFont f = font();
        f.setUnderline(true);
        p.setFont(f);
        p.drawText(rect, Qt::TextDontClip | Qt::TextWrapAnywhere, d->m_preEditString);
    }
}

void TerminalView::paintCells(QPainter &p, QPaintEvent *event) const
{
    QFont f = font();

    const int scrollOffset = verticalScrollBar()->value();

    const int maxRow = d->m_surface->fullSize().height();
    const int startRow = qFloor((qreal) event->rect().y() / d->m_cellSize.height()) + scrollOffset;
    const int endRow = qMin(maxRow,
                            qCeil((event->rect().y() + event->rect().height())
                                  / d->m_cellSize.height())
                                + scrollOffset);

    QList<SearchHit>::const_iterator searchIt
        = std::lower_bound(searchHits().constBegin(),
                           searchHits().constEnd(),
                           startRow,
                           [this](const SearchHit &hit, int value) {
                               return d->m_surface->posToGrid(hit.start).y() < value;
                           });

    for (int cellY = startRow; cellY < endRow; ++cellY) {
        for (int cellX = 0; cellX < d->m_surface->liveSize().width();) {
            const auto cell = d->m_surface->fetchCell(cellX, cellY);

            QRectF cellRect(gridToGlobal({cellX, cellY}),
                            QSizeF{d->m_cellSize.width() * cell.width, d->m_cellSize.height()});

            int numCells = paintCell(p, cellRect, {cellX, cellY}, cell, f, searchIt);

            cellX += numCells;
        }
    }
}

void TerminalView::paintDebugSelection(QPainter &p, const Selection &selection) const
{
    auto s = globalToViewport(gridToGlobal(d->m_surface->posToGrid(selection.start)).toPoint());
    const auto e = globalToViewport(
        gridToGlobal(d->m_surface->posToGrid(selection.end), true).toPoint());

    p.setPen(QPen(Qt::green, 1, Qt::DashLine));
    p.drawLine(s.x(), 0, s.x(), height());
    p.drawLine(0, s.y(), width(), s.y());

    p.setPen(QPen(Qt::red, 1, Qt::DashLine));

    p.drawLine(e.x(), 0, e.x(), height());
    p.drawLine(0, e.y(), width(), e.y());
}

void TerminalView::paintEvent(QPaintEvent *event)
{
    QElapsedTimer t;
    t.start();
    event->accept();
    QPainter p(viewport());

    p.save();

    if (paintLog().isDebugEnabled())
        p.fillRect(event->rect(), QColor::fromRgb(rand() % 60, rand() % 60, rand() % 60));
    else
        p.fillRect(event->rect(), d->m_currentColors[(size_t) WidgetColorIdx::Background]);

    int scrollOffset = verticalScrollBar()->value();
    int offset = -(scrollOffset * d->m_cellSize.height());

    qreal margin = topMargin();

    p.translate(QPointF{0.0, offset + margin});

    paintCells(p, event);
    paintCursor(p);
    paintPreedit(p);

    p.restore();

    p.fillRect(QRectF{{0, 0}, QSizeF{(qreal) width(), topMargin()}},
               d->m_currentColors[(size_t) WidgetColorIdx::Background]);

    if (selectionLog().isDebugEnabled()) {
        if (d->m_selection)
            paintDebugSelection(p, *d->m_selection);
        if (d->m_linkSelection)
            paintDebugSelection(p, *d->m_linkSelection);
    }

    if (paintLog().isDebugEnabled()) {
        QToolTip::showText(this->mapToGlobal(QPoint(width() - 200, 0)),
                           QString("Paint: %1ms").arg(t.elapsed()));
    }

    d->m_sinceLastPaint = QDeadlineTimer(minRefreshInterval);
}

void TerminalView::keyPressEvent(QKeyEvent *event)
{
    // Don't blink during typing
    if (d->m_cursorBlinkTimer.isActive()) {
        d->m_cursorBlinkTimer.start();
        d->m_cursorBlinkState = true;
    }

    if (event->key() == Qt::Key_Control) {
        if (!d->m_linkSelection.has_value() && checkLinkAt(mapFromGlobal(QCursor::pos()))) {
            setCursor(Qt::PointingHandCursor);
        }
    }

    event->accept();

    if (d->m_surface->isInAltScreen()) {
        d->m_surface->sendKey(event);
    } else {
        switch (event->key()) {
        case Qt::Key_PageDown:
            verticalScrollBar()->setValue(qBound(
                0,
                verticalScrollBar()->value() + d->m_surface->liveSize().height(),
                verticalScrollBar()->maximum()));
            break;
        case Qt::Key_PageUp:
            verticalScrollBar()->setValue(qBound(
                0,
                verticalScrollBar()->value() - d->m_surface->liveSize().height(),
                verticalScrollBar()->maximum()));
            break;
        default:
            if (event->key() < Qt::Key_Shift || event->key() > Qt::Key_ScrollLock)
                verticalScrollBar()->setValue(verticalScrollBar()->maximum());
            d->m_surface->sendKey(event);
            break;
        }
    }
}

void TerminalView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control && d->m_linkSelection.has_value()) {
        d->m_linkSelection.reset();
        setCursor(Qt::IBeamCursor);
        updateViewport();
    }
}

void TerminalView::applySizeChange()
{
    QSize newLiveSize = {
        qFloor((qreal) (viewport()->size().width()) / (qreal) d->m_cellSize.width()),
        qFloor((qreal) (viewport()->size().height()) / d->m_cellSize.height()),
    };

    if (newLiveSize.height() <= 0)
        return;

    if (newLiveSize.width() <= 0)
        newLiveSize.setWidth(1);

    if (d->m_surface->liveSize() == newLiveSize)
        return;

    if (resizePty(newLiveSize)) {
        d->m_surface->resize(newLiveSize);
        flushVTerm(true);
    }
}

void TerminalView::updateScrollBars()
{
    int scrollSize = d->m_surface->fullSize().height() - d->m_surface->liveSize().height();
    const bool shouldScroll = verticalScrollBar()->value() == verticalScrollBar()->maximum();
    verticalScrollBar()->setRange(0, scrollSize);
    if (shouldScroll)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    updateViewport();
}

void TerminalView::resizeEvent(QResizeEvent *event)
{
    event->accept();

    // If increasing in size, we'll trigger libvterm to call sb_popline in
    // order to pull lines out of the history.  This will cause the scrollback
    // to decrease in size which reduces the size of the verticalScrollBar.
    // That will trigger a scroll offset increase which we want to ignore.
    d->m_ignoreScroll = true;

    applySizeChange();

    setSelection(std::nullopt);
    d->m_ignoreScroll = false;
}

QRect TerminalView::gridToViewport(QRect rect) const
{
    int offset = verticalScrollBar()->value();

    int startRow = rect.y() - offset;
    int numRows = rect.height();
    int numCols = rect.width();

    QRectF r{rect.x() * d->m_cellSize.width(),
             startRow * d->m_cellSize.height(),
             numCols * d->m_cellSize.width(),
             numRows * d->m_cellSize.height()};

    r.translate(0, topMargin());

    return r.toAlignedRect();
}

QPoint TerminalView::toGridPos(QMouseEvent *event) const
{
    return globalToGrid(QPointF(event->pos()) + QPointF(0, -topMargin() + 0.5));
}

void TerminalView::scheduleViewportUpdate()
{
    if (!d->m_passwordModeActive && d->m_updateRegion)
        viewport()->update(*d->m_updateRegion);
    else
        viewport()->update();

    d->m_updateRegion.reset();
}

void TerminalView::updateViewport()
{
    updateViewportRect({});
}

void TerminalView::updateViewportRect(const QRect &rect)
{
    if (rect.isEmpty())
        d->m_updateRegion = QRegion{viewport()->rect()};
    else if (!d->m_updateRegion)
        d->m_updateRegion = QRegion(rect);
    else
        d->m_updateRegion = d->m_updateRegion->united(rect);

    if (d->m_updateTimer.isActive())
        return;

    if (!d->m_sinceLastPaint.hasExpired()) {
        d->m_updateTimer.start();
        return;
    }

    scheduleViewportUpdate();
}

void TerminalView::focusInEvent(QFocusEvent *)
{
    updateViewport();
    configBlinkTimer();
    selectionChanged(d->m_selection);
    d->m_surface->sendFocus(true);
}
void TerminalView::focusOutEvent(QFocusEvent *)
{
    updateViewport();
    configBlinkTimer();
    d->m_surface->sendFocus(false);
}

void TerminalView::inputMethodEvent(QInputMethodEvent *event)
{
    // Gnome sends empty events when switching virtual desktops, so ignore those.
    if (event->commitString().isEmpty() && event->preeditString().isEmpty()
        && event->attributes().empty() && d->m_preEditString.isEmpty())
        return;

    verticalScrollBar()->setValue(verticalScrollBar()->maximum());

    d->m_preEditString = event->preeditString();

    if (event->commitString().isEmpty()) {
        updateViewport();
        return;
    }

    d->m_surface->sendKey(event->commitString());
}

void TerminalView::mousePressEvent(QMouseEvent *event)
{
    if (d->m_allowMouseTracking) {
        d->m_surface->mouseMove(toGridPos(event), event->modifiers());
        d->m_surface->mouseButton(event->button(), true, event->modifiers());
    }

    d->m_scrollDirection = 0;

    d->m_activeMouseSelect.start = viewportToGlobal(event->pos());

    if (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier) {
        if (d->m_linkSelection) {
            if (event->modifiers() & Qt::ShiftModifier) {
                copyLinkToClipboard();
                return;
            }

            linkActivated(d->m_linkSelection->link);
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (d->m_selection && system_clock::now() - d->m_lastDoubleClick < 500ms) {
            d->m_selectLineMode = true;
            const Selection newSelection{d->m_surface->gridToPos(
                                             {0,
                                              d->m_surface->posToGrid(d->m_selection->start).y()}),
                                         d->m_surface->gridToPos(
                                             {d->m_surface->liveSize().width(),
                                              d->m_surface->posToGrid(d->m_selection->end).y()}),
                                         false};
            setSelection(newSelection);
        } else {
            d->m_selectLineMode = false;
            int pos = d->m_surface->gridToPos(globalToGrid(viewportToGlobal(event->pos())));
            setSelection(Selection{pos, pos, false});
        }
        event->accept();
        updateViewport();
    } else if (event->button() == Qt::RightButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            contextMenuRequested(event->pos());
        } else if (d->m_selection) {
            copyToClipboard();
            setSelection(std::nullopt);
        } else {
            pasteFromClipboard();
        }
    } else if (event->button() == Qt::MiddleButton) {
        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard->supportsSelection()) {
            const QString selectionText = clipboard->text(QClipboard::Selection);
            if (!selectionText.isEmpty())
                d->m_surface->pasteFromClipboard(selectionText);
        } else {
            d->m_surface->pasteFromClipboard(textFromSelection());
        }
    }
}
void TerminalView::mouseMoveEvent(QMouseEvent *event)
{
    if (d->m_allowMouseTracking)
        d->m_surface->mouseMove(toGridPos(event), event->modifiers());

    if (d->m_selection && event->buttons() & Qt::LeftButton) {
        Selection newSelection = *d->m_selection;
        int scrollVelocity = 0;
        if (event->pos().y() < 0) {
            scrollVelocity = (event->pos().y());
        } else if (event->pos().y() > viewport()->height()) {
            scrollVelocity = (event->pos().y() - viewport()->height());
        }

        if ((scrollVelocity != 0) != d->m_scrollTimer.isActive()) {
            if (scrollVelocity != 0)
                d->m_scrollTimer.start();
            else
                d->m_scrollTimer.stop();
        }

        d->m_scrollDirection = scrollVelocity;

        if (d->m_scrollTimer.isActive() && scrollVelocity != 0) {
            const milliseconds scrollInterval = 1000ms / qAbs(scrollVelocity);
            if (d->m_scrollTimer.intervalAsDuration() != scrollInterval)
                d->m_scrollTimer.setInterval(scrollInterval);
        }

        QPoint posBoundedToViewport = event->pos();
        posBoundedToViewport.setX(qBound(0, posBoundedToViewport.x(), viewport()->width()));

        int start = d->m_surface->gridToPos(globalToGrid(d->m_activeMouseSelect.start));
        int newEnd = d->m_surface->gridToPos(globalToGrid(viewportToGlobal(posBoundedToViewport)));

        if (start > newEnd) {
            std::swap(start, newEnd);
        }
        if (start < 0)
            start = 0;

        if (d->m_selectLineMode) {
            newSelection.start = d->m_surface->gridToPos({0, d->m_surface->posToGrid(start).y()});
            newSelection.end = d->m_surface->gridToPos(
                {d->m_surface->liveSize().width(), d->m_surface->posToGrid(newEnd).y()});
        } else {
            newSelection.start = start;
            newSelection.end = newEnd;
        }

        setSelection(newSelection);
    } else if (event->modifiers() & Qt::ControlModifier) {
        checkLinkAt(event->pos());
    } else if (d->m_linkSelection) {
        d->m_linkSelection.reset();
        updateViewport();
    }

    if (d->m_linkSelection) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::IBeamCursor);
    }
}

void TerminalView::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->m_allowMouseTracking) {
        d->m_surface->mouseMove(toGridPos(event), event->modifiers());
        d->m_surface->mouseButton(event->button(), false, event->modifiers());
    }

    d->m_scrollTimer.stop();

    if (d->m_selection && event->button() == Qt::LeftButton) {
        if (d->m_selection->end - d->m_selection->start == 0)
            setSelection(std::nullopt);
        else
            setSelection(Selection{d->m_selection->start, d->m_selection->end, true});
    }
}

void TerminalView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (d->m_allowMouseTracking) {
        d->m_surface->mouseMove(toGridPos(event), event->modifiers());
        d->m_surface->mouseButton(event->button(), true, event->modifiers());
        d->m_surface->mouseButton(event->button(), false, event->modifiers());
    }

    const auto hit = textAt(event->pos());

    setSelection(Selection{hit.start, hit.end, true});

    d->m_lastDoubleClick = system_clock::now();

    event->accept();
}

void TerminalView::wheelEvent(QWheelEvent *event)
{
    verticalScrollBar()->event(event);

    if (!d->m_allowMouseTracking)
        return;

    if (event->angleDelta().ry() > 0)
        d->m_surface->mouseButton(Qt::ExtraButton1, true, event->modifiers());
    else if (event->angleDelta().ry() < 0)
        d->m_surface->mouseButton(Qt::ExtraButton2, true, event->modifiers());
}

bool TerminalView::checkLinkAt(const QPoint &pos)
{
    const TextAndOffsets hit = textAt(pos);

    if (hit.text.size() > 0) {
        QString t = QString::fromUcs4(hit.text.c_str(), hit.text.size()).trimmed();
        auto newLink = toLink(t);
        if (newLink) {
            const LinkSelection newSelection = LinkSelection{{hit.start, hit.end}, newLink.value()};
            if (!d->m_linkSelection || *d->m_linkSelection != newSelection) {
                d->m_linkSelection = newSelection;
                updateViewport();
            }

            return true;
        }
    }

    if (d->m_linkSelection) {
        d->m_linkSelection.reset();
        updateViewport();
    }
    return false;
}

TerminalView::TextAndOffsets TerminalView::textAt(const QPoint &pos) const
{
    auto it = d->m_surface->iteratorAt(globalToGrid(viewportToGlobal(pos)));
    auto itRev = d->m_surface->rIteratorAt(globalToGrid(viewportToGlobal(pos)));

    std::u32string whiteSpaces = U" \t\x00a0";

    const bool inverted = whiteSpaces.find(*it) != std::u32string::npos || *it == 0;

    auto predicate = [inverted, whiteSpaces](const std::u32string::value_type &ch) {
        if (inverted)
            return ch != 0 && whiteSpaces.find(ch) == std::u32string::npos;
        else
            return ch == 0 || whiteSpaces.find(ch) != std::u32string::npos;
    };

    auto itRight = std::find_if(it, d->m_surface->end(), predicate);
    auto itLeft = std::find_if(itRev, d->m_surface->rend(), predicate);

    std::u32string text;
    std::copy(itLeft.base(), it, std::back_inserter(text));
    std::copy(it, itRight, std::back_inserter(text));
    std::transform(text.begin(), text.end(), text.begin(), [](const char32_t &ch) {
        return ch == 0 ? U' ' : ch;
    });

    return {(itLeft.base()).position(), itRight.position(), text};
}

bool TerminalView::event(QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()),
                   d->m_currentColors[(size_t) WidgetColorIdx::Background]);
        return true;
    }

    // TODO: Is this necessary?
    if (event->type() == QEvent::KeyPress) {
        auto k = static_cast<QKeyEvent *>(event);
        keyPressEvent(k);
        return true;
    }
    if (event->type() == QEvent::KeyRelease) {
        auto k = static_cast<QKeyEvent *>(event);
        keyReleaseEvent(k);
        return true;
    }

    return QAbstractScrollArea::event(event);
}

} // namespace TerminalSolution
