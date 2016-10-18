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

#include "refactoroverlay.h"
#include "textdocumentlayout.h"
#include "texteditor.h"

#include <utils/icon.h>

#include <QPainter>

#include <QDebug>

namespace TextEditor {

RefactorOverlay::RefactorOverlay(TextEditor::TextEditorWidget *editor) :
    QObject(editor),
    m_editor(editor),
    m_maxWidth(0),
    m_icon(Utils::Icon({
        {QLatin1String(":/texteditor/images/lightbulbcap.png"), Utils::Theme::PanelTextColorMid},
        {QLatin1String(":/texteditor/images/lightbulb.png"), Utils::Theme::IconsWarningColor}}, Utils::Icon::Tint).icon())
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
    const QSize proposedIconSize = QSize(m_editor->fontMetrics().width(QLatin1Char(' ')) + 3,
                                         cursorRect.height()) * devicePixelRatio;
    const QSize actualIconSize = icon.actualSize(proposedIconSize) / devicePixelRatio;

    const int y = cursorRect.top() + ((cursorRect.height() - actualIconSize.height()) / 2);
    const int x = cursorRect.right();
    marker.rect = QRect(x, y, actualIconSize.width(), actualIconSize.height());

    icon.paint(painter, marker.rect);
    m_maxWidth = qMax(m_maxWidth, x + actualIconSize.width() - int(offset.x()));
}

} // namespace TextEditor
