#ifndef QMLDESIGNER_ABSTRACTSINGLESELECTIONTOOL_H
#define QMLDESIGNER_ABSTRACTSINGLESELECTIONTOOL_H

#include "abstractformeditortool.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT AbstractCustomTool : public QmlDesigner::AbstractFormEditorTool
{
public:
    AbstractCustomTool();

    void selectedItemsChanged(const QList<FormEditorItem *> &itemList) QTC_OVERRIDE;

    virtual QString name() const = 0;

    virtual int wantHandleItem(const ModelNode &modelNode) const = 0;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_ABSTRACTSINGLESELECTIONTOOL_H
