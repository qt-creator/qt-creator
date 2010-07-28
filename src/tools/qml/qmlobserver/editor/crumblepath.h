#ifndef CRUMBLEPATH_H
#define CRUMBLEPATH_H

#include <QWidget>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QResizeEvent);

namespace QmlViewer {

class CrumblePathButton;

class CrumblePath : public QWidget
{
    Q_OBJECT
public:
    explicit CrumblePath(QWidget *parent = 0);
    ~CrumblePath();
    void pushElement(const QString &title);
    void popElement();
    void clear();

signals:
    void elementClicked(int index);
    void elementContextMenuRequested(int index);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void mapClickToIndex();
    void mapContextMenuRequestToIndex();

private:
    void resizeButtons();

private:
    QList<CrumblePathButton*> m_buttons;
};

} // namespace QmlViewer

#endif // CRUMBLEPATH_H
