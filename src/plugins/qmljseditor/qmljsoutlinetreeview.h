#ifndef QMLJSOUTLINETREEVIEW_H
#define QMLJSOUTLINETREEVIEW_H

#include <utils/navigationtreeview.h>

namespace QmlJSEditor {
namespace Internal {

class QmlJSOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    explicit QmlJSOutlineTreeView(QWidget *parent = 0);

    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void collapseAllExceptRoot();
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSOUTLINETREEVIEW_H
