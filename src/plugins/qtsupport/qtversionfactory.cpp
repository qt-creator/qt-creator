/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qtversionfactory.h"
#include "profilereader.h"
#include "baseqtversion.h"

#include <proparser/qmakevfs.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>

using namespace QtSupport;
using namespace QtSupport::Internal;

QtVersionFactory::QtVersionFactory(QObject *parent) :
    QObject(parent)
{
}

QtVersionFactory::~QtVersionFactory()
{
}

BaseQtVersion *QtVersionFactory::createQtVersionFromQMakePath(const Utils::FileName &qmakePath, bool isAutoDetected, const QString &autoDetectionSource, QString *error)
{
    QHash<ProKey, ProString> versionInfo;
    if (!BaseQtVersion::queryQMakeVariables(qmakePath, Utils::Environment::systemEnvironment(),
                                            &versionInfo, error))
        return 0;
    Utils::FileName mkspec = BaseQtVersion::mkspecFromVersionInfo(versionInfo);

    QMakeVfs vfs;
    QMakeGlobals globals;
    globals.setProperties(versionInfo);
    ProMessageHandler msgHandler(false);
    ProFileCacheManager::instance()->incRefCount();
    QMakeParser parser(ProFileCacheManager::instance()->cache(), &vfs, &msgHandler);
    ProFileEvaluator evaluator(&globals, &parser, &vfs, &msgHandler);
    evaluator.loadNamedSpec(mkspec.toString(), false);

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();
    Utils::sort(factories, [](const QtVersionFactory *l, const QtVersionFactory *r) {
        return l->priority() > r->priority();
    });

    foreach (QtVersionFactory *factory, factories) {
        BaseQtVersion *ver = factory->create(qmakePath, &evaluator, isAutoDetected, autoDetectionSource);
        if (ver) {
            ProFileCacheManager::instance()->decRefCount();
            return ver;
        }
    }
    ProFileCacheManager::instance()->decRefCount();
    if (error)
        *error = tr("No factory found for qmake: \"%1\"").arg(qmakePath.toUserOutput());
    return 0;
}
