#include "propertyeditorcontextobject.h"

namespace QmlDesigner {

PropertyEditorContextObject::PropertyEditorContextObject(QObject *parent) :
    QObject(parent), m_isBaseState(false), m_selectionChanged(false), m_backendValues(0)
{

}

} //QmlDesigner
