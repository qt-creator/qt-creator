// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QTextCursor>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QPainterPath)

namespace TextEditor {
class TextEditorWidget;

namespace Internal {

struct OverlaySelection
{
    OverlaySelection() = default;

    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_fg;
    QColor m_bg;
    int m_fixedLength = -1;
    bool m_dropShadow = false;
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

    void update();

    void setAlpha(bool enabled) { m_alpha = enabled; }

    virtual void clear();

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

    bool hasFirstSelectionBeginMoved() const;

protected:
    QTextCursor cursorForSelection(const OverlaySelection &selection) const;
    QTextCursor cursorForIndex(int selectionIndex) const;

private:
    QPainterPath createSelectionPath(const QTextCursor &begin, const QTextCursor &end, const QRect& clip);
    void paintSelection(QPainter *painter, const OverlaySelection &selection, const QRect &clip);
    void fillSelection(QPainter *painter, const OverlaySelection &selection, const QColor &color, const QRect &clip);

    bool m_visible;
    bool m_alpha;
    int m_dropShadowWidth;
    int m_firstSelectionOriginalBegin;
    TextEditorWidget *m_editor;
    QWidget *m_viewport;
    QList<OverlaySelection> m_selections;
};

} // namespace Internal
} // namespace TextEditor
