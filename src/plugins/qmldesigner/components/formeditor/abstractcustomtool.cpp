#include "abstractcustomtool.h"

#include "formeditorview.h"

namespace QmlDesigner {

AbstractCustomTool::AbstractCustomTool(FormEditorView* view)
    : AbstractFormEditorTool(view)
{
}

void AbstractCustomTool::selectedItemsChanged(const QList<FormEditorItem *> &itemList)
{
    view()->changeToSelectionTool();
}


} // namespace QmlDesigner
