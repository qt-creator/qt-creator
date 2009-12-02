/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TEXTEDITOROVERLAY_H
#define TEXTEDITOROVERLAY_H

#include <QtGui/QWidget>
#include "basetexteditor.h"

namespace TextEditor {
namespace Internal {

struct TEXTEDITOR_EXPORT OverlaySelection {
    OverlaySelection():m_fixedLength(-1), m_dropShadow(false){}
    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_fg;
    QColor m_bg;
    int m_fixedLength;
    bool m_dropShadow;
};

class TEXTEDITOR_EXPORT TextEditorOverlay : public QObject
{
Q_OBJECT
BaseTextEditor *m_editor;
QWidget *m_viewport;

public:
QList<OverlaySelection> m_selections;
private:

bool m_visible;
int m_borderWidth;

public:
    TextEditorOverlay(BaseTextEditor *editor);

    QRect rect() const;
    void paint(QPainter *painter, const QRect &clip);
    void fill(QPainter *painter, const QColor &color, const QRect &clip);

    void paintInverted(QPainter *painter, const QRect &clip, const QColor &color);

    bool isVisible() const { return m_visible; }
    void setVisible(bool b);

    void setBorderWidth(int bw) {m_borderWidth = bw; }

    void update();

    void clear();
    void addOverlaySelection(const QTextCursor &cursor, const QColor &fg, const QColor &bg, bool lockSize = false);
    void addOverlaySelection(int begin, int end, const QColor &fg, const QColor &bg, bool lockSize = false, bool dropShadow = false);

    inline bool isEmpty() const { return m_selections.isEmpty(); }

private:
    QPainterPath createSelectionPath(const QTextCursor &begin, const QTextCursor &end, const QRect& clip);
    void paintSelection(QPainter *painter, const OverlaySelection &selection);
    void fillSelection(QPainter *painter, const OverlaySelection &selection, const QColor &color);

};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITOROVERLAY_H
