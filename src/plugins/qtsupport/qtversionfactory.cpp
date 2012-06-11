/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtversionfactory.h"
#include "profilereader.h"
#include "qtversionmanager.h"
#include "baseqtversion.h"

#include <extensionsystem/pluginmanager.h>
#include <QSettings>

using namespace QtSupport;
using namespace QtSupport::Internal;

QtVersionFactory::QtVersionFactory(QObject *parent) :
    QObject(parent)
{

}

QtVersionFactory::~QtVersionFactory()
{

}

bool sortByPriority(QtVersionFactory *a, QtVersionFactory *b)
{
    return a->priority() > b->priority();
}

BaseQtVersion *QtVersionFactory::createQtVersionFromLegacySettings(const Utils::FileName &qmakePath, int id, QSettings *s)
{
    BaseQtVersion *v = createQtVersionFromQMakePath(qmakePath);
    if (!v)
        return 0;
    v->setId(id);
    v->setDisplayName(s->value(QLatin1String("Name")).toString());
    v->restoreLegacySettings(s);
    return v;
}

BaseQtVersion *QtVersionFactory::createQtVersionFromQMakePath(const Utils::FileName &qmakePath, bool isAutoDetected, const QString &autoDetectionSource)
{
    QHash<QString, QString> versionInfo;
    bool success = BaseQtVersion::queryQMakeVariables(qmakePath, &versionInfo);
    if (!success)
        return 0;
    Utils::FileName mkspec = BaseQtVersion::mkspecFromVersionInfo(versionInfo);

    QMakeGlobals globals;
    globals.properties = versionInfo;
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    QMakeParser parser(ProFileCacheManager::instance()->cache(), &msgHandler);
    ProFileEvaluator evaluator(&globals, &parser, &msgHandler);
    if (ProFile *pro = parser.parsedProFile(mkspec.toString() + QLatin1String("/qmake.conf"))) {
        evaluator.setCumulative(false);
        evaluator.accept(pro, QMakeEvaluator::LoadProOnly);
        pro->deref();
    }

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();
    qSort(factories.begin(), factories.end(), &sortByPriority);

    foreach (QtVersionFactory *factory, factories) {
        BaseQtVersion *ver = factory->create(qmakePath, &evaluator, isAutoDetected, autoDetectionSource);
        if (ver) {
            ProFileCacheManager::instance()->decRefCount();
            return ver;
        }
    }
    ProFileCacheManager::instance()->decRefCount();
    return 0;
}
