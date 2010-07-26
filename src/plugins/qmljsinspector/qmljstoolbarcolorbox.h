#ifndef TOOLBARCOLORBOX_H
#define TOOLBARCOLORBOX_H

#include <QLabel>
#include <QColor>
#include <QPoint>

QT_FORWARD_DECLARE_CLASS(QContextMenuEvent);
QT_FORWARD_DECLARE_CLASS(QAction);

namespace QmlJSInspector {

class ToolBarColorBox : public QLabel
{
    Q_OBJECT
public:
    explicit ToolBarColorBox(QWidget *parent = 0);
    void setColor(const QColor &color);
    void setInnerBorderColor(const QColor &color);
    void setOuterBorderColor(const QColor &color);

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
    QAction *m_copyHexColorAction;
    QColor m_color;

    QColor m_borderColorOuter;
    QColor m_borderColorInner;

};

} // namespace QmlJSInspector

#endif // TOOLBARCOLORBOX_H
