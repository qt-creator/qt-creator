#ifndef REFACTOROVERLAY_H
#define REFACTOROVERLAY_H

#include "basetexteditor.h"
#include <QTextCursor>
#include <QObject>
#include <QIcon>

namespace TextEditor {
namespace Internal {

struct  TEXTEDITOR_EXPORT RefactorMarker {
    inline bool isValid() const { return !cursor.isNull(); }
    QTextCursor cursor;
    QString tooltip;
    QIcon icon;
    mutable QRect rect; // used to cache last drawing positin in document coordinates
    QVariant data;
};

typedef QList<RefactorMarker> RefactorMarkers;

class  TEXTEDITOR_EXPORT RefactorOverlay : public QObject
{
    Q_OBJECT
public:
    explicit RefactorOverlay(BaseTextEditor *editor);

    bool isEmpty() const { return m_markers.isEmpty(); }
    void paint(QPainter *painter, const QRect &clip);

    void setMarkers(const RefactorMarkers &markers) { m_markers = markers; }
    RefactorMarkers markers() const { return m_markers; }

    void clear() { m_markers.clear(); }

    RefactorMarker markerAt(const QPoint &pos) const;

private:
    void paintMarker(const RefactorMarker& marker, int position, QPainter *painter, const QRect &clip);
    RefactorMarkers m_markers;
    BaseTextEditor *m_editor;
    int m_maxWidth;
    QIcon m_icon;

};

}
}

#endif // REFACTOROVERLAY_H
