#include "refactoroverlay.h"
#include <QPainter>
#include <QTextBlock>
#include "basetextdocumentlayout.h"
#include <QDebug>

using namespace TextEditor::Internal;

RefactorOverlay::RefactorOverlay(TextEditor::BaseTextEditor *editor) :
    QObject(editor),
    m_editor(editor),
    m_maxWidth(0),
    m_icon(QLatin1String(":/texteditor/images/refactormarker.png"))
{
}

void RefactorOverlay::paint(QPainter *painter, const QRect &clip)
{
    QTextBlock lastBlock;
    int position = 0;
    m_maxWidth = 0;
    for (int i = 0; i < m_markers.size(); ++i) {

        // position counts how many refactor markers are in a single block
        if (m_markers.at(i).cursor.block() != lastBlock) {
            lastBlock = m_markers.at(i).cursor.block();
            position = 0;
        } else {
            position++;
        }

        paintMarker(m_markers.at(i), position, painter, clip);
    }

    if (BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(m_editor->document()->documentLayout())) {
        documentLayout->setRequiredWidth(m_maxWidth);
    }

}

RefactorMarker RefactorOverlay::markerAt(const QPoint &pos) const
{
    QPointF offset = m_editor->contentOffset();
    foreach(const RefactorMarker &marker, m_markers) {
        if (marker.rect.translated(offset.toPoint()).contains(pos))
            return marker;
    }
    return RefactorMarker();
}

void RefactorOverlay::paintMarker(const RefactorMarker& marker, int position,
                                  QPainter *painter, const QRect &clip)
{
    QPointF offset = m_editor->contentOffset();
    QRectF geometry = m_editor->blockBoundingGeometry(marker.cursor.block()).translated(offset);

    if (geometry.top() > clip.bottom() + 10 || geometry.bottom() < clip.top() - 10)
        return; // marker not visible

    QTextCursor cursor = marker.cursor;
    cursor.movePosition(QTextCursor::EndOfLine);
    QRect r = m_editor->cursorRect(cursor);

    QIcon icon = marker.icon;
    if (icon.isNull())
        icon = m_icon;

    QSize sz = icon.actualSize(QSize(INT_MAX, r.height()));

    int x = r.right() + position * sz.width();
    marker.rect = QRect(x, r.bottom() - sz.height(), sz.width(), sz.height()).translated(-offset.toPoint());
    icon.paint(painter, marker.rect);
    m_maxWidth = qMax((qreal)m_maxWidth, x + sz.width() - offset.x());
}


