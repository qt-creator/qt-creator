/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemomanager.h"

#include "maemoconstants.h"
#include "maemodeploystepfactory.h"
#include "maemodeviceconfigurations.h"
#include "maemoglobal.h"
#include "maemopackagecreationfactory.h"
#include "maemopublishingwizardfactories.h"
#include "maemoqemumanager.h"
#include "maemorunfactories.h"
#include "maemosettingspages.h"
#include "maemotemplatesmanager.h"
#include "maemotoolchain.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qtversionmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

using namespace ExtensionSystem;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
    namespace Internal {

MaemoManager *MaemoManager::m_instance = 0;

MaemoManager::MaemoManager()
    : QObject(0)
    , m_runControlFactory(new MaemoRunControlFactory(this))
    , m_runConfigurationFactory(new MaemoRunConfigurationFactory(this))
    , m_packageCreationFactory(new MaemoPackageCreationFactory(this))
    , m_deployStepFactory(new MaemoDeployStepFactory(this))
    , m_deviceConfigurationsSettingsPage(new MaemoDeviceConfigurationsSettingsPage(this))
    , m_qemuSettingsPage(new MaemoQemuSettingsPage(this))
    , m_publishingFactoryFremantleFree(new MaemoPublishingWizardFactoryFremantleFree(this))
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    MaemoQemuManager::instance(this);
    MaemoDeviceConfigurations::instance(this);
    MaemoTemplatesManager::instance(this);

    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->addObject(m_runControlFactory);
    pluginManager->addObject(m_runConfigurationFactory);
    pluginManager->addObject(m_packageCreationFactory);
    pluginManager->addObject(m_deployStepFactory);
    pluginManager->addObject(m_deviceConfigurationsSettingsPage);
    pluginManager->addObject(m_qemuSettingsPage);
    pluginManager->addObject(m_publishingFactoryFremantleFree);
}

MaemoManager::~MaemoManager()
{
    // TODO: Remove in reverse order of adding.
    PluginManager *pluginManager = PluginManager::instance();
    pluginManager->removeObject(m_runControlFactory);
    pluginManager->removeObject(m_runConfigurationFactory);
    pluginManager->removeObject(m_deployStepFactory);
    pluginManager->removeObject(m_packageCreationFactory);
    pluginManager->removeObject(m_deviceConfigurationsSettingsPage);
    pluginManager->removeObject(m_qemuSettingsPage);
    pluginManager->removeObject(m_publishingFactoryFremantleFree);

    m_instance = 0;
}

MaemoManager &MaemoManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

bool MaemoManager::isValidMaemoQtVersion(const QtVersion *version) const
{
    QString path = QDir::cleanPath(version->qmakeCommand());
    path.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));
    QDir dir(path);
    const QByteArray target = dir.dirName().toAscii();
    dir.cdUp(); dir.cdUp();
    const QString madAdminCommand(dir.absolutePath() + QLatin1String("/bin/mad-admin"));
    if (!QFileInfo(madAdminCommand).exists())
        return false;

    QProcess madAdminProc;
    const QStringList arguments(QLatin1String("list"));
    MaemoGlobal::callMaddeShellScript(madAdminProc, dir.absolutePath(),
        madAdminCommand, arguments);
    if (!madAdminProc.waitForStarted() || !madAdminProc.waitForFinished())
        return false;

    madAdminProc.setReadChannel(QProcess::StandardOutput);
    while (madAdminProc.canReadLine()) {
        const QByteArray &line = madAdminProc.readLine();
        if (line.contains(target)
            && (line.contains("(installed)") || line.contains("(default)")))
            return true;
    }
    return false;
}

ToolChain* MaemoManager::maemoToolChain(const QtVersion *version) const
{
    QString targetRoot = QDir::cleanPath(version->qmakeCommand());
    targetRoot.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));
    return new MaemoToolChain(targetRoot);
}

    } // namespace Internal
} // namespace Qt4ProjectManager
