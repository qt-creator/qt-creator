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
    m_maxWidth = 0;
    for (int i = 0; i < m_markers.size(); ++i) {
        paintMarker(m_markers.at(i), painter, clip);
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

void RefactorOverlay::paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip)
{
    QPointF offset = m_editor->contentOffset();
    QRectF geometry = m_editor->blockBoundingGeometry(marker.cursor.block()).translated(offset);

    if (geometry.top() > clip.bottom() + 10 || geometry.bottom() < clip.top() - 10)
        return; // marker not visible

    QTextCursor cursor = marker.cursor;

    QRect r = m_editor->cursorRect(cursor);

    QIcon icon = marker.icon;
    if (icon.isNull())
        icon = m_icon;

    QSize sz = icon.actualSize(QSize(m_editor->fontMetrics().width(QLatin1Char(' '))+2, r.height()));

    int x = r.right();
    marker.rect = QRect(x, r.top(), sz.width(), sz.height());

    icon.paint(painter, marker.rect);
    m_maxWidth = qMax((qreal)m_maxWidth, x + sz.width() - offset.x());
}


