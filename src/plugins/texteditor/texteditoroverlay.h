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

#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QTextCursor>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace TextEditor {
class NameMangler;
class TextEditorWidget;

namespace Internal {

struct OverlaySelection
{
    OverlaySelection():m_fixedLength(-1), m_dropShadow(false){}

    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_fg;
    QColor m_bg;
    int m_fixedLength;
    bool m_dropShadow;
};

class TextEditorOverlay : public QObject
{
    Q_OBJECT
public:
    TextEditorOverlay(TextEditorWidget *editor);

    QRect rect() const;
    void paint(QPainter *painter, const QRect &clip);
    void fill(QPainter *painter, const QColor &color, const QRect &clip);

    bool isVisible() const { return m_visible; }
    void setVisible(bool b);

    inline void hide() { setVisible(false); }
    inline void show() { setVisible(true); }

    void setBorderWidth(int bw) {m_borderWidth = bw; }

    void update();

    void setAlpha(bool enabled) { m_alpha = enabled; }

    void clear();

    enum OverlaySelectionFlags {
        LockSize = 1,
        DropShadow = 2,
        ExpandBegin = 4
    };

    void addOverlaySelection(const QTextCursor &cursor, const QColor &fg, const QColor &bg,
                             uint overlaySelectionFlags = 0);
    void addOverlaySelection(int begin, int end, const QColor &fg, const QColor &bg,
                             uint overlaySelectionFlags = 0);

    const QList<OverlaySelection> &selections() const { return m_selections; }

    inline bool isEmpty() const { return m_selections.isEmpty(); }

    inline int dropShadowWidth() const { return m_dropShadowWidth; }

    bool hasCursorInSelection(const QTextCursor &cursor) const;

    void mapEquivalentSelections();
    void updateEquivalentSelections(const QTextCursor &cursor);
    void setNameMangler(const QList<NameMangler *> &manglers);
    void mangle();

    bool hasFirstSelectionBeginMoved() const;

private:
    QPainterPath createSelectionPath(const QTextCursor &begin, const QTextCursor &end, const QRect& clip);
    void paintSelection(QPainter *painter, const OverlaySelection &selection);
    void fillSelection(QPainter *painter, const OverlaySelection &selection, const QColor &color);
    int selectionIndexForCursor(const QTextCursor &cursor) const;
    QString selectionText(int selectionIndex) const;
    QTextCursor assembleCursorForSelection(int selectionIndex) const;

    bool m_visible;
    bool m_alpha;
    int m_borderWidth;
    int m_dropShadowWidth;
    int m_firstSelectionOriginalBegin;
    TextEditorWidget *m_editor;
    QWidget *m_viewport;
    QList<OverlaySelection> m_selections;
    QVector<QList<int> > m_equivalentSelections;
    QList<NameMangler *> m_manglers;
};

} // namespace Internal
} // namespace TextEditor
