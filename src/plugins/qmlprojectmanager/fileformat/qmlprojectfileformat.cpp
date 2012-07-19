/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
