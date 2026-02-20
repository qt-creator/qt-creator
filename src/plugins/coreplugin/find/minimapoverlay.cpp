// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minimapoverlay.h"

#include <utils/plaintextedit/plaintextedit.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>
#include <QTextBlock>
#include <QWheelEvent>

using namespace Core;
using namespace Utils;

MinimapOverlay::MinimapOverlay(PlainTextEdit *editor)
    : QWidget(editor)
    , m_editor(editor)
{
    QTC_ASSERT(editor, return);
    m_doc = editor->document();
    m_vScroll = editor->verticalScrollBar();

    m_scrollbarDefaultWidth = editor->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    editor->setEditorTextMargin("Core.MinimapWidth", Qt::RightEdge, m_minimapWidth);

    editor->installEventFilter(this);

    connect(m_doc, &QTextDocument::contentsChange, this, &MinimapOverlay::onDocumentChanged);
    connect(
        m_doc->documentLayout(),
        &QAbstractTextDocumentLayout::update,
        this,
        &MinimapOverlay::onDocumentChanged);

    // Minimap is outside the editor's viewport and needs a repaint when the editor is painted
    connect(editor, &PlainTextEdit::updateRequest, this, [this] { update(); }, Qt::QueuedConnection);

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(30);
    connect(&m_updateTimer, &QTimer::timeout, this, &MinimapOverlay::updateImage);

    setAutoFillBackground(true);

    scheduleUpdate();
}

MinimapOverlay::~MinimapOverlay()
{
    if (m_editor)
        m_editor->setEditorTextMargin("Core.MinimapWidth", Qt::RightEdge, 0);
}

void MinimapOverlay::paintMinimap(QPainter *painter) const
{
    if (m_minimap.isNull() || !m_vScroll)
        return;

    QColor bg = m_editor->palette().brush(QPalette::Base).color();
    painter->fillRect(rect(), bg);

    const QRect geo = rect().adjusted(1, 0, 1, 0);

    ThumbGeometry tg = computeThumbGeometry();
    const qreal &scrollFraction = tg.scrollFraction;
    const QRect &thumbRect = tg.rect;

    if (m_minimap.height() > geo.height()) {
        int srcY = qRound(scrollFraction * (m_minimap.height() - geo.height()));
        const QRect srcRect(0, srcY, geo.width(), geo.height());

        painter->drawImage(geo.topLeft(), m_minimap.copy(srcRect));
    } else {
        painter->drawImage(geo.topLeft(), m_minimap);
    }

    QColor overlay = creatorColor(Theme::Token_Foreground_Default);
    overlay.setAlphaF(m_minimapAlpha);
    painter->setCompositionMode(
        creatorTheme()->colorScheme() == Qt::ColorScheme::Dark ? QPainter::CompositionMode_Lighten
                                                               : QPainter::CompositionMode_Darken);
    painter->fillRect(thumbRect, overlay);

    QPen pen;
    pen.setWidthF(m_editor->devicePixelRatio());
    pen.setColor(creatorColor(Theme::SplitterColor));
    painter->setPen(pen);
    painter->drawLine(rect().topLeft(), rect().bottomLeft());
}

void MinimapOverlay::scheduleUpdate()
{
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void MinimapOverlay::onDocumentChanged()
{
    scheduleUpdate();
}

static inline void updatePixel(quint32 *dst, QRgb bgPremul, const QColor &fg, bool blend = false)
{
    if (!blend) {
        *dst = bgPremul;
        return;
    }

    // Convert the foreground colour to premultiplied ARGB32 once.
    const QRgb fgPremul = qPremultiply(fg.rgba());

    const quint32 aF = (fgPremul >> 24) & 0xff;
    if (aF == 0) {
        *dst = bgPremul; // fully transparent foreground -> keep bg
        return;
    }
    if (aF == 255) {
        *dst = fgPremul; // fully opaque foreground -> replace bg
        return;
    }

    const quint32 aB = (bgPremul >> 24) & 0xff;

    // src‑over (premultiplied)
    const quint32 aOut = aF + aB * (255 - aF) / 255;
    const quint32 rOut = ((fgPremul >> 16) & 0xff) + ((bgPremul >> 16) & 0xff) * (255 - aF) / 255;
    const quint32 gOut = ((fgPremul >> 8) & 0xff) + ((bgPremul >> 8) & 0xff) * (255 - aF) / 255;
    const quint32 bOut = (fgPremul & 0xff) + (bgPremul & 0xff) * (255 - aF) / 255;

    *dst = (aOut << 24) | (rOut << 16) | (gOut << 8) | bOut;
}

void MinimapOverlay::updateImage()
{
    if (!m_editor || !m_editor->isVisible())
        return;

    int docLineCount = [this]() {
        int lineCount = 0;
        for (QTextBlock block = m_doc->firstBlock(); block.isValid(); block = block.next()) {
            if (!block.isVisible())
                continue;
            ++lineCount;
        }
        return lineCount;
    }();

    if (m_editor->centerOnScroll()) {
        // if center cursor on scroll is enabled, it is possible to scroll past the end of the
        // document, so we need to add enough lines to fill the viewport
        auto viewportHeight = m_editor->viewport()->height();
        auto lineSpacing = m_editor->editorLayout()->lineSpacing();
        docLineCount += viewportHeight / lineSpacing;
    }

    const int imageWidth = m_minimapWidth - 2;
    const int lineDup = m_pixelsPerLine;

    const int imageHeight = miniLineHeight() * docLineCount;
    if (imageWidth <= 0 || imageHeight <= 0)
        return;

    QImage img(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    const QColor bgColor = QColor(Qt::transparent);
    const quint32 bgPremul = qPremultiply(bgColor.rgba());

    const QFontMetricsF fm(m_editor->font());
    const int spaceWidth = qCeil(fm.horizontalAdvance(QLatin1Char(' ')));

    quint32 *dstLine = reinterpret_cast<quint32 *>(img.bits());

    auto advanceToNextLine = [&]() {
        for (int i = 1; i < lineDup; ++i)
            memcpy(dstLine + i * imageWidth, dstLine, imageWidth * sizeof(quint32));

        dstLine += lineDup * imageWidth;

        memset(dstLine, bgPremul, m_lineGap * imageWidth * sizeof(quint32));
        dstLine += m_lineGap * imageWidth;
    };

    QColor defaultTextColor = m_editor->palette().brush(QPalette::Text).color();
    for (QTextBlock block = m_doc->firstBlock(); block.isValid(); block = block.next()) {
        if (!block.isVisible())
            continue;

        const QTextLayout *layout = block.layout();
        QVector<QTextLayout::FormatRange> formats = layout->formats();
        std::sort(
            formats.begin(),
            formats.end(),
            [](const QTextLayout::FormatRange &a, const QTextLayout::FormatRange &b) {
                return a.start < b.start;
            });

        const QString text = block.text();

        int formatIndex = 0;
        int currentFormatEnd = formats.isEmpty() ? block.length() : formats.first().start;
        QColor curFg = defaultTextColor;

        int dstX = 0;
        for (int i = 0; i < text.length() && dstX < imageWidth; ++i) {
            if (i >= currentFormatEnd) {
                if (formatIndex >= formats.size()) {
                    currentFormatEnd = block.length();
                    curFg = defaultTextColor;
                } else {
                    const QTextLayout::FormatRange &format = formats[formatIndex];
                    if (i < format.start) {
                        currentFormatEnd = format.start;
                        curFg = defaultTextColor;
                    } else {
                        const QTextCharFormat &fmt = format.format;
                        if (fmt.foreground().style() != Qt::NoBrush)
                            curFg = fmt.foreground().color();
                        else
                            curFg = defaultTextColor;

                        currentFormatEnd = format.start + format.length;
                        ++formatIndex;
                    }
                }
            }
            QChar ch = text.at(i);
            if (ch.isSpace()) {
                updatePixel(&dstLine[dstX], bgPremul, curFg, false);
                ++dstX;
                continue;
            }

            if (ch == QLatin1Char('\t')) {
                int nextTabX = ((dstX * spaceWidth) / spaceWidth + 1) * spaceWidth;
                while (dstX < imageWidth && dstX < nextTabX) {
                    updatePixel(&dstLine[dstX], bgPremul, curFg, false);
                    ++dstX;
                }
                continue;
            }

            // printable character
            updatePixel(&dstLine[dstX], bgPremul, curFg, true);
            ++dstX;
        }

        while (dstX < imageWidth) {
            updatePixel(&dstLine[dstX], bgPremul, curFg, false);
            ++dstX;
        }

        advanceToNextLine();
    }

    m_minimap = img;
    update();
}


void MinimapOverlay::doMove()
{
    QMetaObject::invokeMethod(
        this,
        [this] {
            const int x = m_editor->width() - m_scrollbarDefaultWidth - m_minimapWidth;
            move(x, 0);
        },
        Qt::QueuedConnection);
}

void MinimapOverlay::doResize()
{
    resize(m_minimapWidth, m_editor->height());
}

MinimapOverlay::ThumbGeometry MinimapOverlay::computeThumbGeometry() const
{
    const qreal scale = miniLineHeight() / qreal(m_vScroll->singleStep());
    const qreal scrollFraction = m_vScroll->maximum() > 0
                                     ? qreal(m_vScroll->value()) / m_vScroll->maximum()
                                     : qreal(m_vScroll->value()) / m_vScroll->pageStep();

    const QRect geo = rect().adjusted(1, 0, 1, 0);
    const int thumbHeight = qRound(m_vScroll->pageStep() * scale);
    const int thumbTop
        = geo.top()
          + qRound(scrollFraction * (qMin(m_minimap.height(), geo.height()) - thumbHeight));

    ThumbGeometry tg;
    tg.scrollFraction = scrollFraction;
    tg.rect = QRect(rect().left(), thumbTop, m_minimapWidth, thumbHeight);
    return tg;
}

void MinimapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (!m_vScroll)
        return;

    const QPoint clickPos = event->pos();

    const ThumbGeometry tg = computeThumbGeometry();
    const QRect &thumb = tg.rect;

    if (thumb.contains(clickPos)) {
        m_dragging = true;
        m_dragStartValue = m_vScroll->value();
        m_dragOffset = clickPos.y() - thumb.top();

        event->accept();
        return;
    }

    const int delta = (clickPos.y() < thumb.top()) ? -m_vScroll->pageStep() : m_vScroll->pageStep();
    const int scrollPosition = m_vScroll->value() + delta;
    m_vScroll->setValue(qBound(m_vScroll->minimum(), scrollPosition, m_vScroll->maximum()));

    event->accept();
}

void MinimapOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_vScroll || !m_dragging)
        return;

    const int scrollPosition = minimapPixelPosToRangeValue(event->pos().y() - m_dragOffset);
    m_vScroll->setValue(qBound(m_vScroll->minimum(), scrollPosition, m_vScroll->maximum()));

    event->accept();
}

void MinimapOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_vScroll)
        return;
    if (!m_dragging)
        return;

    m_dragging = false;
    event->accept();
}

int MinimapOverlay::minimapPixelPosToRangeValue(int pos) const
{
    const QRect geo = rect().adjusted(1, 0, 1, 0);
    const ThumbGeometry tg = computeThumbGeometry();
    const QRect &thumb = tg.rect;

    const int thumbMin = geo.top();
    const int thumbMax = geo.top() + qMin(m_minimap.height(), geo.height()) - thumb.height() + 1;

    return QStyle::sliderValueFromPosition(
        m_vScroll->minimum(), m_vScroll->maximum(), pos - thumbMin, thumbMax - thumbMin, false);
}

void MinimapOverlay::wheelEvent(QWheelEvent *event)
{
    if (!m_vScroll)
        return;

    QPointF mappedPos = m_vScroll->mapFromGlobal(event->globalPosition());
    mappedPos.setX(m_scrollbarDefaultWidth / 2);
    QWheelEvent forwarded(
        mappedPos,
        event->globalPosition(),
        event->pixelDelta(),
        event->angleDelta(),
        event->buttons(),
        event->modifiers(),
        event->phase(),
        event->inverted());

    QApplication::sendEvent(m_vScroll, &forwarded);
    event->accept();
}

void MinimapOverlay::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    paintMinimap(&painter);
}

bool MinimapOverlay::eventFilter(QObject *object, QEvent *event)
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
        doResize();
        doMove();
        show();
        scheduleUpdate();
        break;
    case QEvent::Hide:
        hide();
        break;
    default:
        break;
    }

    return QWidget::eventFilter(object, event);
}
