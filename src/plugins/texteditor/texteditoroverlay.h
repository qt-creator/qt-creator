#ifndef TEXTEDITOROVERLAY_H
#define TEXTEDITOROVERLAY_H

#include <QtGui/QWidget>
#include "basetexteditor.h"

namespace TextEditor {
namespace Internal {

struct TEXTEDITOR_EXPORT OverlaySelection {
    OverlaySelection():m_fixedLength(-1){}
    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_fg;
    QColor m_bg;
    int m_fixedLength;
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
    void addOverlaySelection(int begin, int end, const QColor &fg, const QColor &bg, bool lockSize = false);

    inline bool isEmpty() const { return m_selections.isEmpty(); }

private:
    QPainterPath createSelectionPath(const QTextCursor &begin, const QTextCursor &end, const QRect& clip);
    void paintSelection(QPainter *painter, const QTextCursor &begin, const QTextCursor &end,
                        const QColor &fg, const QColor &bg);
    void fillSelection(QPainter *painter, const QTextCursor &begin, const QTextCursor &end,
                        const QColor &color);

};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITOROVERLAY_H
