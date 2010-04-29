#include "propertyeditorcontextobject.h"

namespace QmlDesigner {

PropertyEditorContextObject::PropertyEditorContextObject(QObject *parent) :
    QObject(parent), m_backendValues(0), m_isBaseState(false), m_selectionChanged(false)
{

}

} //QmlDesigner
