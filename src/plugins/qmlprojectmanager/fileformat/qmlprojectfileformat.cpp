#include "qmlprojectfileformat.h"
#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <qdeclarative.h>

namespace QmlProjectManager {

void QmlProjectFileFormat::registerDeclarativeTypes()
{
    QML_REGISTER_NOCREATE_TYPE(QmlProjectManager::QmlProjectContentItem);
    QML_REGISTER_TYPE(QmlProject,1,0,Project,QmlProjectManager::QmlProjectItem);

    QML_REGISTER_TYPE(QmlProject,1,0,QmlFiles,QmlProjectManager::QmlFileFilterItem);
    QML_REGISTER_TYPE(QmlProject,1,0,JavaScriptFiles,QmlProjectManager::JsFileFilterItem);
    QML_REGISTER_TYPE(QmlProject,1,0,ImageFiles,QmlProjectManager::ImageFileFilterItem);
    QML_REGISTER_TYPE(QmlProject,1,0,CssFiles,QmlProjectManager::CssFileFilterItem);
}

} // namespace QmlProjectManager
