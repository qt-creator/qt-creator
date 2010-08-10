#include "qmljsoutlinetreeview.h"

namespace QmlJSEditor {
namespace Internal {

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    // see also CppOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setExpandsOnDoubleClick(false);

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(InternalMove);
}

} // namespace Internal
} // namespace QmlJSEditor
