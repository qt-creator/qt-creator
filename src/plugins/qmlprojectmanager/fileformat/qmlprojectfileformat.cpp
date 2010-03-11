#include "qmlprojectfileformat.h"
#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <qdeclarative.h>

namespace QmlProjectManager {

void QmlProjectFileFormat::registerDeclarativeTypes()
{
    qmlRegisterType<QmlProjectManager::QmlProjectContentItem>();
    qmlRegisterType<QmlProjectManager::QmlProjectItem>("QmlProject",1,0,"Project");

    qmlRegisterType<QmlProjectManager::QmlFileFilterItem>("QmlProject",1,0,"QmlFiles");
    qmlRegisterType<QmlProjectManager::JsFileFilterItem>("QmlProject",1,0,"JavaScriptFiles");
    qmlRegisterType<QmlProjectManager::ImageFileFilterItem>("QmlProject",1,0,"ImageFiles");
    qmlRegisterType<QmlProjectManager::CssFileFilterItem>("QmlProject",1,0,"CssFiles");
}

} // namespace QmlProjectManager
