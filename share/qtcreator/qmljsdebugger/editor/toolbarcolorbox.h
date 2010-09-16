#ifndef ToolBarColorBox_H
#define ToolBarColorBox_H

#include <QLabel>
#include <QColor>
#include <QPoint>

QT_FORWARD_DECLARE_CLASS(QContextMenuEvent);
QT_FORWARD_DECLARE_CLASS(QAction);

namespace QmlObserver {

class ToolBarColorBox : public QLabel
{
    Q_OBJECT
public:
    explicit ToolBarColorBox(QWidget *parent = 0);
    void setColor(const QColor &color);

protected:
    void contextMenuEvent(QContextMenuEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
private slots:
    void copyColorToClipboard();

private:
    QPixmap createDragPixmap(int size = 24) const;

private:
    bool m_dragStarted;
    QPoint m_dragBeginPoint;
    QAction *m_copyHexColor;
    QColor m_color;

};

} // namespace QmlObserver

#endif // ToolBarColorBox_H
