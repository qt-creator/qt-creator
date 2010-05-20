/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "maemomanager.h"

#include "maemoconstants.h"
#include "maemodeviceconfigurations.h"
#include "maemopackagecreationfactory.h"
#include "maemorunfactories.h"
#include "maemosettingspage.h"
#include "maemotoolchain.h"
#include "qemuruntimemanager.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qtversionmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace Qt4ProjectManager {
    namespace Internal {

MaemoManager *MaemoManager::m_instance = 0;

MaemoManager::MaemoManager()
    : QObject(0)
    , m_runControlFactory(new MaemoRunControlFactory(this))
    , m_runConfigurationFactory(new MaemoRunConfigurationFactory(this))
    , m_packageCreationFactory(new MaemoPackageCreationFactory(this))
    , m_settingsPage(new MaemoSettingsPage(this))
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    MaemoDeviceConfigurations::instance(this);

    ExtensionSystem::PluginManager *pluginManager
        = ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(m_runControlFactory);
    pluginManager->addObject(m_runConfigurationFactory);
    pluginManager->addObject(m_packageCreationFactory);
    pluginManager->addObject(m_settingsPage);
}

MaemoManager::~MaemoManager()
{
    ExtensionSystem::PluginManager *pluginManager
        = ExtensionSystem::PluginManager::instance();
    pluginManager->removeObject(m_runControlFactory);
    pluginManager->removeObject(m_runConfigurationFactory);
    pluginManager->removeObject(m_packageCreationFactory);
    pluginManager->removeObject(m_settingsPage);

    m_instance = 0;
}

MaemoManager &MaemoManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

void MaemoManager::init()
{
    m_qemuRuntimeManager = new QemuRuntimeManager(this);
}

bool
MaemoManager::isValidMaemoQtVersion(const Qt4ProjectManager::QtVersion *version) const
{
    QString path = QDir::cleanPath(version->qmakeCommand());
    path = path.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));

    QDir dir(path);
    dir.cdUp(); dir.cdUp();

    QFile file(dir.absolutePath() + QLatin1String("/cache/madde.conf"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString &target = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString &line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("target"))
                && line.endsWith(target)) {
                    return true;
            }
        }
    }
    return false;
}

ProjectExplorer::ToolChain*
MaemoManager::maemoToolChain(const QtVersion *version) const
{
    QString targetRoot = QDir::cleanPath(version->qmakeCommand());
    targetRoot.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));
    return new MaemoToolChain(targetRoot);
}

    } // namespace Internal
} // namespace Qt4ProjectManager
