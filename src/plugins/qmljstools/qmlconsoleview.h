#ifndef QMLCONSOLEVIEW_H
#define QMLCONSOLEVIEW_H

#include <QTreeView>

namespace QmlJSTools {
namespace Internal {

class QmlConsoleView : public QTreeView
{
    Q_OBJECT
public:
    QmlConsoleView(QWidget *parent);

public slots:
    void onScrollToBottom();

protected:
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *e);
    void drawBranches(QPainter *painter, const QRect &rect,
                      const QModelIndex &index) const;
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void onRowActivated(const QModelIndex &index);

private:
    void copyToClipboard(const QModelIndex &index);
    bool canShowItemInTextEditor(const QModelIndex &index);
};

} // Internal
} // QmlJSTools

#endif // QMLCONSOLEVIEW_H
