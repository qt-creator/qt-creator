#include "qmljsoutlinetreeview.h"
#include "qmloutlinemodel.h"

#include <utils/annotateditemdelegate.h>
#include <QtGui/QMenu>

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

    setRootIsDecorated(false);

    Utils::AnnotatedItemDelegate *itemDelegate = new Utils::AnnotatedItemDelegate(this);
    itemDelegate->setDelimiter(QLatin1String(" "));
    itemDelegate->setAnnotationRole(QmlOutlineModel::AnnotationRole);
    setItemDelegateForColumn(0, itemDelegate);
}

void QmlJSOutlineTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
        return;

    QMenu contextMenu;

    contextMenu.addAction(tr("Expand All"), this, SLOT(expandAll()));
    contextMenu.addAction(tr("Collapse All"), this, SLOT(collapseAllExceptRoot()));

    contextMenu.exec(event->globalPos());

    event->accept();
}

void QmlJSOutlineTreeView::collapseAllExceptRoot()
{
    if (!model())
        return;
    const QModelIndex rootElementIndex = model()->index(0, 0, rootIndex());
    int rowCount = model()->rowCount(rootElementIndex);
    for (int i = 0; i < rowCount; ++i) {
        collapse(model()->index(i, 0, rootElementIndex));
    }
}

} // namespace Internal
} // namespace QmlJSEditor
