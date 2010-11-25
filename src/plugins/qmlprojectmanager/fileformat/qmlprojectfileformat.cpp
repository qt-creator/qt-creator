#include "qmlprojectfileformat.h"
#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <qdeclarative.h>

namespace QmlProjectManager {

void QmlProjectFileFormat::registerDeclarativeTypes()
{
    qmlRegisterType<QmlProjectManager::QmlProjectContentItem>();
    qmlRegisterType<QmlProjectManager::QmlProjectItem>("QmlProject",1,0,"Project");
    qmlRegisterType<QmlProjectManager::QmlProjectItem>("QmlProject",1,1,"Project");

    qmlRegisterType<QmlProjectManager::QmlFileFilterItem>("QmlProject",1,0,"QmlFiles");
    qmlRegisterType<QmlProjectManager::QmlFileFilterItem>("QmlProject",1,1,"QmlFiles");
    qmlRegisterType<QmlProjectManager::JsFileFilterItem>("QmlProject",1,0,"JavaScriptFiles");
    qmlRegisterType<QmlProjectManager::JsFileFilterItem>("QmlProject",1,1,"JavaScriptFiles");
    qmlRegisterType<QmlProjectManager::ImageFileFilterItem>("QmlProject",1,0,"ImageFiles");
    qmlRegisterType<QmlProjectManager::ImageFileFilterItem>("QmlProject",1,1,"ImageFiles");
    qmlRegisterType<QmlProjectManager::CssFileFilterItem>("QmlProject",1,0,"CssFiles");
    qmlRegisterType<QmlProjectManager::CssFileFilterItem>("QmlProject",1,1,"CssFiles");
    qmlRegisterType<QmlProjectManager::OtherFileFilterItem>("QmlProject",1,1,"Files");
}

} // namespace QmlProjectManager
