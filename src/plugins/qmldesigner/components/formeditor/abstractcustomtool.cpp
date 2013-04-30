#include "abstractcustomtool.h"

#include "formeditorview.h"

namespace QmlDesigner {

AbstractCustomTool::AbstractCustomTool()
    : AbstractFormEditorTool(0)
{
}

void AbstractCustomTool::selectedItemsChanged(const QList<FormEditorItem *> & /*itemList*/)
{
    view()->changeToSelectionTool();
}


} // namespace QmlDesigner
