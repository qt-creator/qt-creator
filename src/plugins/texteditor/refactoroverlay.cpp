/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "refactoroverlay.h"
#include "textdocumentlayout.h"
#include "texteditor.h"

#include <QPainter>

#include <QDebug>

namespace TextEditor {

RefactorOverlay::RefactorOverlay(TextEditor::TextEditorWidget *editor) :
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

    if (TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(m_editor->document()->documentLayout()))
        documentLayout->setRequiredWidth(m_maxWidth);

}

RefactorMarker RefactorOverlay::markerAt(const QPoint &pos) const
{
    foreach (const RefactorMarker &marker, m_markers) {
        if (marker.rect.contains(pos))
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

} // namespace TextEditor
