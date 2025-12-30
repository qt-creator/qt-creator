// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minimapoverlay.h"

#include <utils/plaintextedit/plaintextedit.h>
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

    editor->setEditorTextMargin("Core.MinimapWidth", Qt::RightEdge, m_minimapWidth);

    editor->installEventFilter(this);
    doResize();
    doMove();
    setVisible(m_vScroll->isVisible());

    connect(m_doc, &QTextDocument::contentsChange, this, &MinimapOverlay::onDocumentChanged);
    connect(
        m_doc->documentLayout(),
        &QAbstractTextDocumentLayout::update,
        this,
        &MinimapOverlay::onDocumentChanged);

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(30);
    connect(&m_updateTimer, &QTimer::timeout, this, &MinimapOverlay::updateImage);

    scheduleUpdate();
}

void MinimapOverlay::paintMinimap(QPainter *painter) const
{
    if (m_minimap.isNull() || !m_vScroll)
        return;

    painter->fillRect(rect(), creatorColor(Theme::Token_Background_Default));

    const QRect geo = rect().adjusted(1, 0, 1, 0);

    const qreal scale = miniLineHeight() / qreal(m_vScroll->singleStep());
    const qreal scrollFraction = m_vScroll->maximum() > 0
                                     ? qreal(m_vScroll->value()) / m_vScroll->maximum()
                                     : qreal(m_vScroll->value()) / m_vScroll->pageStep();

    if (m_minimap.height() > geo.height()) {
        int srcY = qRound(scrollFraction * (m_minimap.height() - geo.height()));
        const QRect srcRect(0, srcY, geo.width(), geo.height());

        painter->drawImage(geo, m_minimap, srcRect);
    } else {
        painter->drawImage(geo.topLeft(), m_minimap);
    }

    const int thumbHeight = qRound(m_vScroll->pageStep() * scale);
    const int thumbTop
        = geo.top()
          + qRound(scrollFraction * (qMin(m_minimap.height(), geo.height()) - thumbHeight));
    QRect thumbRect(rect().left(), thumbTop, m_minimapWidth, thumbHeight);

    QColor overlay = creatorColor(Theme::Token_Foreground_Default);
    overlay.setAlphaF(m_minimapAlpha);
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

    const int docLineCount = [this]() {
        int lineCount = 0;
        for (QTextBlock block = m_doc->firstBlock(); block.isValid(); block = block.next()) {
            if (!block.isVisible())
                continue;
            ++lineCount;
        }
        return lineCount;
    }();

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

    for (QTextBlock block = m_doc->firstBlock(); block.isValid(); block = block.next()) {
        if (!block.isVisible())
            continue;

        const QTextLayout *layout = block.layout();
        QVector<QTextLayout::FormatRange> formats = layout->formats();
        if (formats.isEmpty()) {
            QTextLayout::FormatRange fr;
            fr.start = 0;
            fr.length = block.length();
            fr.format = block.charFormat();
            formats.append(fr);
        } else {
            std::sort(
                formats.begin(),
                formats.end(),
                [](const QTextLayout::FormatRange &a, const QTextLayout::FormatRange &b) {
                    return a.start < b.start;
                });
        }

        const QString text = block.text();
        int fmtIdx = 0;
        int fmtPos = formats.first().start;
        int fmtEnd = fmtPos + formats.first().length;

        const QTextCharFormat &cf = formats[fmtIdx].format;
        QColor curFg = cf.foreground().color();
        QColor curBg = cf.background().color();

        int dstX = 0;
        for (int i = 0; i < text.length() && dstX < imageWidth; ++i) {
            while (i >= fmtEnd && fmtIdx + 1 < formats.size()) {
                ++fmtIdx;
                fmtPos = formats[fmtIdx].start;
                fmtEnd = fmtPos + formats[fmtIdx].length;
                const QTextCharFormat &cf = formats[fmtIdx].format;
                curFg = cf.foreground().color();
                curBg = cf.background().color();
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
}


void MinimapOverlay::doMove()
{
    QTimer::singleShot(0, [this]{
        QPoint point = parentWidget()->mapFromGlobal(m_vScroll->mapToGlobal(m_vScroll->pos()));
        point.setX(point.x() - m_minimapWidth);

        move(point);
    });
}

void MinimapOverlay::doResize()
{
    QSize size = m_vScroll->size();

    const int h = m_editor->viewport()->height();
    const int w = size.width() + m_minimapWidth;
    resize(w, h);
}

void MinimapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (!m_vScroll)
        return;

    QPoint mappedPos = m_vScroll->mapFromGlobal(event->globalPosition()).toPoint();
    mappedPos.setX(m_vScroll->width() / 2);

    // Compute the minimap thumb rect
    const QRect geo = rect().adjusted(1, 0, 1, 0);
    const qreal scale = miniLineHeight() / qreal(m_vScroll->singleStep());
    const qreal scrollFraction = m_vScroll->maximum() > 0
                                     ? qreal(m_vScroll->value()) / m_vScroll->maximum()
                                     : qreal(m_vScroll->value()) / m_vScroll->pageStep();
    const int thumbHeight = qRound(m_vScroll->pageStep() * scale);
    const int thumbTop
        = geo.top()
          + qRound(scrollFraction * (qMin(m_minimap.height(), geo.height()) - thumbHeight));
    const QRect minimapThumbRect(rect().left(), thumbTop, rect().width(), thumbHeight);

    // Get the scrollbar thumb rect
    QStyleOptionSlider opt;
    opt.initFrom(m_vScroll);
    opt.orientation = m_vScroll->orientation();
    opt.minimum = m_vScroll->minimum();
    opt.maximum = m_vScroll->maximum();
    opt.pageStep = m_vScroll->pageStep();
    opt.singleStep = m_vScroll->singleStep();
    opt.sliderPosition = m_vScroll->value();
    opt.sliderValue = m_vScroll->value();
    opt.rect = m_vScroll->rect();

    const QRect thumbRect = m_vScroll->style()->subControlRect(
        QStyle::CC_ScrollBar,
        &opt,
        QStyle::SC_ScrollBarSlider,
        m_vScroll);

    if (thumbRect.contains(mappedPos) || !minimapThumbRect.contains(mappedPos)) {
        QMouseEvent forwarded(
            event->type(),
            mappedPos,
            event->globalPosition(),
            event->button(),
            event->buttons(),
            event->modifiers());

        QApplication::sendEvent(m_vScroll, &forwarded);
    } else {
        // Inside the minimap thumb rect, but outside the real thumb
        QPoint fakePos = thumbRect.center();
        QMouseEvent fakePress(
            event->type(),
            fakePos,
            event->globalPosition(),
            event->button(),
            event->buttons(),
            event->modifiers());

        QApplication::sendEvent(m_vScroll, &fakePress);
    }

    event->accept();
}

void MinimapOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_vScroll)
        return;

    QPointF mappedPos = m_vScroll->mapFromGlobal(event->globalPosition());
    mappedPos.setX(m_vScroll->width() / 2);
    QMouseEvent forwarded(
        event->type(),
        mappedPos,
        event->globalPosition(),
        event->button(),
        event->buttons(),
        event->modifiers());

    QApplication::sendEvent(m_vScroll, &forwarded);
    event->accept();
}

void MinimapOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_vScroll)
        return;

    QPointF mappedPos = m_vScroll->mapFromGlobal(event->globalPosition());
    mappedPos.setX(m_vScroll->width() / 2);
    QMouseEvent forwarded(
        event->type(),
        mappedPos,
        event->globalPosition(),
        event->button(),
        event->buttons(),
        event->modifiers());

    QApplication::sendEvent(m_vScroll, &forwarded);
    event->accept();
}

void MinimapOverlay::wheelEvent(QWheelEvent *event)
{
    if (!m_vScroll)
        return;

    QPointF mappedPos = m_vScroll->mapFromGlobal(event->globalPosition());
    mappedPos.setX(m_vScroll->width() / 2);
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

    QMetaObject::invokeMethod(this, QOverload<>::of(&QWidget::update), Qt::QueuedConnection);
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
        break;
    case QEvent::Hide:
        hide();
        break;
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        break;
    default:
        break;
    }

    // Minimap is outside the editor's viewport and needs a repaint when the editor is painted
    if (event->type() == QEvent::Paint)
        QMetaObject::invokeMethod(this, QOverload<>::of(&QWidget::update), Qt::QueuedConnection);

    return QWidget::eventFilter(object, event);
}
