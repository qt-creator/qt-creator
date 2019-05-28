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
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace QtSupport;
using namespace QtSupport::Internal;

static QList<QtVersionFactory *> g_qtVersionFactories;

QtVersionFactory::QtVersionFactory()
{
    g_qtVersionFactories.append(this);
}

QtVersionFactory::~QtVersionFactory()
{
    g_qtVersionFactories.removeOne(this);
}

const QList<QtVersionFactory *> QtVersionFactory::allQtVersionFactories()
{
    return g_qtVersionFactories;
}

bool QtVersionFactory::canRestore(const QString &type)
{
    return type == m_supportedType;
}

BaseQtVersion *QtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return nullptr);
    QTC_ASSERT(m_creator, return nullptr);
    BaseQtVersion *version = create();
    version->fromMap(data);
    return version;
}

BaseQtVersion *QtVersionFactory::createQtVersionFromQMakePath(const Utils::FilePath &qmakePath, bool isAutoDetected, const QString &autoDetectionSource, QString *error)
{
    QHash<ProKey, ProString> versionInfo;
    if (!BaseQtVersion::queryQMakeVariables(qmakePath, Utils::Environment::systemEnvironment(),
                                            &versionInfo, error))
        return 0;
    Utils::FilePath mkspec = BaseQtVersion::mkspecFromVersionInfo(versionInfo);

    QMakeVfs vfs;
    QMakeGlobals globals;
    globals.setProperties(versionInfo);
    ProMessageHandler msgHandler(false);
    ProFileCacheManager::instance()->incRefCount();
    QMakeParser parser(ProFileCacheManager::instance()->cache(), &vfs, &msgHandler);
    ProFileEvaluator evaluator(&globals, &parser, &vfs, &msgHandler);
    evaluator.loadNamedSpec(mkspec.toString(), false);

    QList<QtVersionFactory *> factories = g_qtVersionFactories;
    Utils::sort(factories, [](const QtVersionFactory *l, const QtVersionFactory *r) {
        return l->priority() > r->priority();
    });

    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return nullptr;

    SetupData setup;
    setup.config = evaluator.values("CONFIG");
    setup.platforms = evaluator.values("QMAKE_PLATFORM"); // It's a list in general.
    setup.isQnx = !evaluator.value("QNX_CPUDIR").isEmpty();

    foreach (QtVersionFactory *factory, factories) {
        if (!factory->m_restrictionChecker || factory->m_restrictionChecker(setup)) {
            BaseQtVersion *ver = factory->create();
            QTC_ASSERT(ver, continue);
            ver->setupQmakePathAndId(qmakePath);
            ver->setAutoDetectionSource(autoDetectionSource);
            ver->setIsAutodetected(isAutoDetected);
            ProFileCacheManager::instance()->decRefCount();
            return ver;
        }
    }
    ProFileCacheManager::instance()->decRefCount();
    if (error) {
        *error = QCoreApplication::translate("QtSupport::QtVersionFactory",
                    "No factory found for qmake: \"%1\"").arg(qmakePath.toUserOutput());
    }
    return 0;
}

BaseQtVersion *QtVersionFactory::create() const
{
    QTC_ASSERT(m_creator, return nullptr);
    BaseQtVersion *version = m_creator();
    version->m_factory = this;
    return version;
}

QString QtVersionFactory::supportedType() const
{
    return m_supportedType;
}

BaseQtVersion *QtVersionFactory::cloneQtVersion(const BaseQtVersion *source)
{
    QTC_ASSERT(source, return nullptr);
    const QString sourceType = source->type();
    for (QtVersionFactory *factory : g_qtVersionFactories) {
        if (factory->m_supportedType == sourceType) {
            BaseQtVersion *version = factory->create();
            QTC_ASSERT(version, return nullptr);
            version->fromMap(source->toMap());
            return version;
        }
    }
    QTC_CHECK(false);
    return nullptr;
}

void QtVersionFactory::setQtVersionCreator(const std::function<BaseQtVersion *()> &creator)
{
    m_creator = creator;
}

void QtVersionFactory::setRestrictionChecker(const std::function<bool(const SetupData &)> &checker)
{
    m_restrictionChecker = checker;
}

void QtVersionFactory::setSupportedType(const QString &type)
{
    m_supportedType = type;
}

void QtVersionFactory::setPriority(int priority)
{
    m_priority = priority;
}
