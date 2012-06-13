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

#include "s60manager.h"
//#include "qtversionmanager.h"

//#include "s60devicespreferencepane.h"
#include "s60devicerunconfiguration.h"
#include "s60createpackagestep.h"
#include "s60deployconfiguration.h"
#include "s60deploystep.h"
#include "s60runcontrolfactory.h"
#include "s60devicedebugruncontrol.h"
#include "symbianidevice.h"
#include "symbianidevicefactory.h"

#include "qt4symbiantargetfactory.h"
#include "s60publishingwizardfactories.h"

#include "gccetoolchain.h"
#include "rvcttoolchain.h"
#include "symbianqtversionfactory.h"

#include <symbianutils/symbiandevicemanager.h>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <debugger/debuggerconstants.h>
#include <utils/qtcassert.h>

#include <QMainWindow>

#include <QDir>

namespace Qt4ProjectManager {
namespace Internal {

S60Manager *S60Manager::m_instance = 0;

// ======== Parametrizable Factory for RunControls, depending on the configuration
// class and mode.

template <class RunControl, class RunConfiguration>
        class RunControlFactory : public ProjectExplorer::IRunControlFactory
{
public:
    RunControlFactory(ProjectExplorer::RunMode mode, const QString &name, QObject *parent = 0) :
    IRunControlFactory(parent), m_mode(mode), m_name(name) {}

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const
    {
        return mode == m_mode && qobject_cast<RunConfiguration *>(runConfiguration);
    }

    ProjectExplorer::RunControl* create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        ProjectExplorer::RunMode mode)
    {
        RunConfiguration *rc = qobject_cast<RunConfiguration *>(runConfiguration);
        QTC_ASSERT(rc && mode == m_mode, return 0);
        return new RunControl(rc, mode);
    }

    QString displayName() const {
        return m_name;
    }

private:
    const ProjectExplorer::RunMode m_mode;
    const QString m_name;
};

// ======== S60Manager

S60Manager *S60Manager::instance() { return m_instance; }

S60Manager::S60Manager(QObject *parent) : QObject(parent)
{
    m_instance = this;

    addAutoReleasedObject(new GcceToolChainFactory);
    addAutoReleasedObject(new RvctToolChainFactory);

    addAutoReleasedObject(new S60DeviceRunConfigurationFactory);
    addAutoReleasedObject(new S60RunControlFactory(ProjectExplorer::NormalRunMode,
                                                 tr("Run on Device"), parent));
    addAutoReleasedObject(new S60CreatePackageStepFactory);
    addAutoReleasedObject(new S60DeployStepFactory);

    addAutoReleasedObject(new S60DeviceDebugRunControlFactory);
    addAutoReleasedObject(new Qt4SymbianTargetFactory);
    addAutoReleasedObject(new S60DeployConfigurationFactory);

    addAutoReleasedObject(new S60PublishingWizardFactoryOvi);
    addAutoReleasedObject(new SymbianQtVersionFactory);

    addAutoReleasedObject(new Internal::SymbianIDeviceFactory);

    connect(Core::ICore::mainWindow(), SIGNAL(deviceChange()),
            SymbianUtils::SymbianDeviceManager::instance(), SLOT(update()));

    SymbianUtils::SymbianDeviceManager *dm = SymbianUtils::SymbianDeviceManager::instance();
    connect(dm, SIGNAL(deviceAdded(SymbianUtils::SymbianDevice)),
            this, SLOT(symbianDeviceAdded(SymbianUtils::SymbianDevice)));
    connect(dm, SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
            this, SLOT(symbianDeviceRemoved(SymbianUtils::SymbianDevice)));
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

void S60Manager::delayedInitialize()
{
    handleQtVersionChanges();
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(handleQtVersionChanges()));
}

void S60Manager::symbianDeviceRemoved(const SymbianUtils::SymbianDevice &d)
{
    handleSymbianDeviceStateChange(d, ProjectExplorer::IDevice::DeviceDisconnected);
}

void S60Manager::symbianDeviceAdded(const SymbianUtils::SymbianDevice &d)
{
    handleSymbianDeviceStateChange(d, ProjectExplorer::IDevice::DeviceReadyToUse);
}

void S60Manager::handleQtVersionChanges()
{
    bool symbianQtFound = false;
    Core::Id symbianDeviceId;
    QList<QtSupport::BaseQtVersion  *> versionList = QtSupport::QtVersionManager::instance()->versions();
    foreach (QtSupport::BaseQtVersion *v, versionList) {
        if (v->platformName() != QLatin1String(QtSupport::Constants::SYMBIAN_PLATFORM))
            continue;

        symbianQtFound = true;
        break;
    }

    ProjectExplorer::DeviceManager *dm = ProjectExplorer::DeviceManager::instance();
    for (int i = 0; i < dm->deviceCount(); ++i) {
        ProjectExplorer::IDevice::ConstPtr dev = dm->deviceAt(i);
        if (dev->type() != SymbianIDeviceFactory::deviceType())
            continue;

        symbianDeviceId = dev->id();
        break;
    }

    if (symbianQtFound && !symbianDeviceId.isValid())
        dm->addDevice(ProjectExplorer::IDevice::Ptr(new SymbianIDevice));
    if (!symbianQtFound && symbianDeviceId.isValid())
        dm->removeDevice(symbianDeviceId);
}

void S60Manager::handleSymbianDeviceStateChange(const SymbianUtils::SymbianDevice &d, ProjectExplorer::IDevice::DeviceState s)
{
    ProjectExplorer::DeviceManager *dm = ProjectExplorer::DeviceManager::instance();
    for (int i = 0; i < dm->deviceCount(); ++i) {
        ProjectExplorer::IDevice::ConstPtr dev = dm->deviceAt(i);
        const SymbianIDevice *sdev = dynamic_cast<const SymbianIDevice *>(dev.data());
        if (!sdev || sdev->communicationChannel() != SymbianIDevice::CommunicationCodaSerialConnection)
            continue;
        if (sdev->serialPortName() != d.portName())
            continue;

        SymbianIDevice *newDev = new SymbianIDevice(*sdev); // Get a new device to replace the current one
        newDev->setDeviceState(s);
        dm->addDevice(ProjectExplorer::IDevice::Ptr(newDev));
        break;
    }
}

void S60Manager::addAutoReleasedObject(QObject *o)
{
    ExtensionSystem::PluginManager::instance()->addObject(o);
    m_pluginObjects.push_back(o);
}

} // namespace internal
} // namespace qt4projectmanager
