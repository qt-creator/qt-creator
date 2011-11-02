/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "s60manager.h"
//#include "qtversionmanager.h"

//#include "s60devicespreferencepane.h"
#include "s60emulatorrunconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "s60createpackagestep.h"
#include "s60deployconfiguration.h"
#include "s60deploystep.h"
#include "s60runcontrolfactory.h"
#include "s60devicedebugruncontrol.h"

#include "qt4symbiantargetfactory.h"
#include "s60publishingwizardfactories.h"

#include "gccetoolchain.h"
#include "rvcttoolchain.h"
#include "winscwtoolchain.h"
#include "symbianqtversionfactory.h"

#include <symbianutils/symbiandevicemanager.h>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QMainWindow>

#include <QtCore/QDir>

namespace Qt4ProjectManager {
namespace Internal {

S60Manager *S60Manager::m_instance = 0;

// ======== Parametrizable Factory for RunControls, depending on the configuration
// class and mode.

template <class RunControl, class RunConfiguration>
        class RunControlFactory : public ProjectExplorer::IRunControlFactory
{
public:
    explicit RunControlFactory(const QString &mode,
                               const QString &name,
                               QObject *parent = 0) :
    IRunControlFactory(parent), m_mode(mode), m_name(name) {}

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const {
        return (mode == m_mode)
                && (qobject_cast<RunConfiguration *>(runConfiguration) != 0);
    }

    ProjectExplorer::RunControl* create(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) {
        RunConfiguration *rc = qobject_cast<RunConfiguration *>(runConfiguration);
        QTC_ASSERT(rc && mode == m_mode, return 0);
        return new RunControl(rc, mode);
    }

    QString displayName() const {
        return m_name;
    }

    ProjectExplorer::RunConfigWidget *createConfigurationWidget(ProjectExplorer::RunConfiguration *) {
        return 0;
    }

private:
    const QString m_mode;
    const QString m_name;
};

// ======== S60Manager

S60Manager *S60Manager::instance() { return m_instance; }

S60Manager::S60Manager(QObject *parent) : QObject(parent)
{
    m_instance = this;

    addAutoReleasedObject(new GcceToolChainFactory);
    addAutoReleasedObject(new RvctToolChainFactory);
    addAutoReleasedObject(new WinscwToolChainFactory);

    addAutoReleasedObject(new S60EmulatorRunConfigurationFactory);
    addAutoReleasedObject(new RunControlFactory<S60EmulatorRunControl, S60EmulatorRunConfiguration>
                          (QLatin1String(ProjectExplorer::Constants::RUNMODE),
                           tr("Run in Emulator"), parent));
    addAutoReleasedObject(new S60DeviceRunConfigurationFactory);
    addAutoReleasedObject(new S60RunControlFactory(QLatin1String(ProjectExplorer::Constants::RUNMODE),
                                                 tr("Run on Device"), parent));
    addAutoReleasedObject(new S60CreatePackageStepFactory);
    addAutoReleasedObject(new S60DeployStepFactory);

    addAutoReleasedObject(new S60DeviceDebugRunControlFactory);
    addAutoReleasedObject(new Qt4SymbianTargetFactory);
    addAutoReleasedObject(new S60DeployConfigurationFactory);

    addAutoReleasedObject(new S60PublishingWizardFactoryOvi);
    addAutoReleasedObject(new SymbianQtVersionFactory);

    connect(Core::ICore::instance()->mainWindow(), SIGNAL(deviceChange()),
            SymbianUtils::SymbianDeviceManager::instance(), SLOT(update()));
}

S60Manager::~S60Manager()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    for (int i = m_pluginObjects.size() - 1; i >= 0; i--) {
        pm->removeObject(m_pluginObjects.at(i));
        delete m_pluginObjects.at(i);
    }
}

QString S60Manager::platform(const ProjectExplorer::ToolChain *tc)
{
    if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::SymbianOS)
        return QString();
    QString target = tc->defaultMakeTarget();
    return target.right(target.lastIndexOf(QLatin1Char('-')));
}

void S60Manager::addAutoReleasedObject(QObject *o)
{
    ExtensionSystem::PluginManager::instance()->addObject(o);
    m_pluginObjects.push_back(o);
}

} // namespace internal
} // namespace qt4projectmanager
