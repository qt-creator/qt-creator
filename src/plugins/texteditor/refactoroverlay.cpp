// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "refactoroverlay.h"
#include "textdocumentlayout.h"
#include "texteditor.h"

#include <utils/algorithm.h>
#include <utils/utilsicons.h>

#include <QPainter>

#include <QDebug>

namespace TextEditor {

RefactorOverlay::RefactorOverlay(TextEditor::TextEditorWidget *editor) :
    QObject(editor),
    m_editor(editor),
    m_maxWidth(0),
    m_icon(Utils::Icons::CODEMODEL_FIXIT.icon())
{
}

void RefactorOverlay::paint(QPainter *painter, const QRect &clip)
{
    const auto firstBlock = m_editor->blockForVerticalOffset(clip.top());
    const int firstBlockNumber = firstBlock.isValid() ? firstBlock.blockNumber() : 0;
    const auto lastBlock = m_editor->blockForVerticalOffset(clip.bottom());
    const int lastBlockNumber = lastBlock.isValid() ? lastBlock.blockNumber()
                                                    : m_editor->blockCount() - 1;

    m_maxWidth = 0;
    for (const RefactorMarker &marker : std::as_const(m_markers)) {
        const int markerBlockNumber = marker.cursor.block().blockNumber();
        if (markerBlockNumber < firstBlockNumber)
            continue;
        if (markerBlockNumber > lastBlockNumber)
            continue;
        paintMarker(marker, painter, clip);
    }

    if (auto documentLayout = qobject_cast<TextDocumentLayout*>(m_editor->document()->documentLayout()))
        documentLayout->setRequiredWidth(m_maxWidth);
}

RefactorMarker RefactorOverlay::markerAt(const QPoint &pos) const
{
    for (const auto &marker : m_markers) {
        if (marker.rect.contains(pos))
            return marker;
    }
    return RefactorMarker();
}

void RefactorOverlay::paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip)
{
    if (!marker.cursor.block().isVisible())
        return; // block containing marker not visible

    const QPointF offset = m_editor->contentOffset();
    const QRectF geometry = m_editor->blockBoundingGeometry(marker.cursor.block()).translated(offset);

    if (geometry.top() > clip.bottom() + 10 || geometry.bottom() < clip.top() - 10)
        return; // marker not visible

    const QTextCursor cursor = marker.cursor;
    const QRect cursorRect = m_editor->cursorRect(cursor);

    QIcon icon = marker.icon;
    if (icon.isNull())
        icon = m_icon;

    const qreal devicePixelRatio = painter->device()->devicePixelRatio();
    const QSize proposedIconSize =
        QSize(m_editor->fontMetrics().horizontalAdvance(QLatin1Char(' ')) + 3,
              cursorRect.height()) * devicePixelRatio;
    const QSize actualIconSize = icon.actualSize(proposedIconSize);

    const int y = cursorRect.top() + ((cursorRect.height() - actualIconSize.height()) / 2);
    const int x = cursorRect.right();
    marker.rect = QRect(x, y, actualIconSize.width(), actualIconSize.height());

    icon.paint(painter, marker.rect);
    m_maxWidth = qMax(m_maxWidth, x + actualIconSize.width() - int(offset.x()));
}

RefactorMarkers RefactorMarker::filterOutType(const RefactorMarkers &markers, const Utils::Id &type)
{
    return Utils::filtered(markers, [type](const RefactorMarker &marker) {
        return marker.type != type;
    });
}

} // namespace TextEditor
