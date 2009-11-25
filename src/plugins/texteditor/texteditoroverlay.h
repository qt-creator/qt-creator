#ifndef TEXTEDITOROVERLAY_H
#define TEXTEDITOROVERLAY_H

#include <QtGui/QWidget>
#include "basetexteditor.h"

namespace TextEditor {
namespace Internal {

struct TEXTEDITOR_EXPORT OverlaySelection {
    QTextCursor m_cursor_begin;
    QTextCursor m_cursor_end;
    QColor m_color;
};

class TEXTEDITOR_EXPORT TextEditorOverlay : public QObject
{
Q_OBJECT
BaseTextEditor *m_editor;
QWidget *m_viewport;

QList<OverlaySelection> m_selections;

bool m_visible;
int m_borderWidth;

public:
    TextEditorOverlay(BaseTextEditor *editor);

    QRect rect() const;
    void paint(QPainter *painter, const QRect &clip);

    bool isVisible() const { return m_visible; }
    void setVisible(bool b);

    void setBorderWidth(int bw) {m_borderWidth = bw; }

    void update();

    void clear();
    void addOverlaySelection(const QTextCursor &cursor, const QColor &color);
    void addOverlaySelection(int begin, int end, const QColor &color);

    inline bool isEmpty() const { return m_selections.isEmpty(); }

private:
    void paintSelection(QPainter *painter, const QTextCursor &begin, const QTextCursor &end, const QColor &color);

};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITOROVERLAY_H
