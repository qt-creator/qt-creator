/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "devicemanagertest.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <remotelinux/linuxdevice.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

#include <cstdlib>
#include <iostream>

using namespace ProjectExplorer;
using namespace RemoteLinux;

#define DEV_MGR_CHECK(cond) QTC_ASSERT(cond, exit(1))

DeviceManagerTest::DeviceManagerTest(QObject *parent) : QObject(parent)
{
}

void DeviceManagerTest::run()
{
    DeviceManager * const devMgr = DeviceManager::instance("magicTestToken");
    DEV_MGR_CHECK(m_slotsCalled.isEmpty());
    connect(devMgr, SIGNAL(deviceAdded(Core::Id)), SLOT(handleDeviceAdded(Core::Id)));
    connect(devMgr, SIGNAL(deviceRemoved(Core::Id)), SLOT(handleDeviceRemoved(Core::Id)));
    connect(devMgr, SIGNAL(deviceUpdated(Core::Id)), SLOT(handleDeviceUpdated(Core::Id)));
    connect(devMgr, SIGNAL(updated()), SLOT(handleUpdated()));
    connect(devMgr, SIGNAL(deviceListChanged()), SLOT(handleDeviceListChanged()));

    std::cout << "Initial add." << std::endl;
    m_currentId = m_currentUpdateId = Core::Id("id1");
    LinuxDevice::Ptr device = LinuxDevice::create("blubb", "mytype",
        LinuxDevice::Hardware, IDevice::AutoDetected, m_currentId);
    devMgr->addDevice(device);
    DEV_MGR_CHECK(devMgr->deviceCount() == 1);
    DEV_MGR_CHECK(devMgr->defaultDevice("mytype") == device);
    DEV_MGR_CHECK(m_slotsCalled.count() == 2);
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotDeviceAdded));
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotUpdated));

    std::cout << "Add with different id, but same name." << std::endl;
    m_slotsCalled.clear();
    m_currentId = Core::Id("id2");
    m_currentUpdateId = Core::Id("doesnotexist");
    device = LinuxDevice::create("blubb", "mytype",
        LinuxDevice::Hardware, IDevice::AutoDetected, m_currentId);
    devMgr->addDevice(device);
    DEV_MGR_CHECK(devMgr->deviceCount() == 2);
    DEV_MGR_CHECK(devMgr->defaultDevice("mytype")->id() == Core::Id("id1"));
    DEV_MGR_CHECK(m_slotsCalled.count() == 2);
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotDeviceAdded));
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotUpdated));
    DEV_MGR_CHECK(devMgr->deviceAt(1)->displayName() == "blubb (2)");

    std::cout << "Add with same id." << std::endl;
    m_slotsCalled.clear();
    m_currentId = m_currentUpdateId = Core::Id("id1");
    device = LinuxDevice::create("blubbblubb", "mytype",
        LinuxDevice::Hardware, IDevice::AutoDetected, m_currentId);
    devMgr->addDevice(device);
    DEV_MGR_CHECK(devMgr->deviceCount() == 2);
    DEV_MGR_CHECK(devMgr->defaultDevice("mytype")->id() == Core::Id("id1"));
    DEV_MGR_CHECK(m_slotsCalled.count() == 2);
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotDeviceUpdated));
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotUpdated));
    DEV_MGR_CHECK(devMgr->deviceAt(0)->displayName() == "blubbblubb");

    std::cout << "Remove." << std::endl;
    m_slotsCalled.clear();
    m_currentId = Core::Id("id1");
    m_currentUpdateId = Core::Id("id2");
    devMgr->removeDevice(m_currentId);
    DEV_MGR_CHECK(devMgr->deviceCount() == 1);
    DEV_MGR_CHECK(devMgr->defaultDevice("mytype")->id() == Core::Id("id2"));
    DEV_MGR_CHECK(m_slotsCalled.count() == 3);
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotDeviceRemoved));
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotDeviceUpdated));
    DEV_MGR_CHECK(m_slotsCalled.contains(SlotUpdated));

    std::cout << "All tests finished successfully." << std::endl;
    qApp->quit();
}

void DeviceManagerTest::handleDeviceAdded(Core::Id id)
{
    DEV_MGR_CHECK(id == m_currentId);
    m_slotsCalled << SlotDeviceAdded;
}

void DeviceManagerTest::handleDeviceRemoved(Core::Id id)
{
    DEV_MGR_CHECK(id == m_currentId);
    m_slotsCalled << SlotDeviceRemoved;
}

void DeviceManagerTest::handleDeviceUpdated(Core::Id id)
{
    DEV_MGR_CHECK(id == m_currentUpdateId);
    m_slotsCalled << SlotDeviceUpdated;
}

void DeviceManagerTest::handleDeviceListChanged()
{
    qWarning("deviceListChanged() unexpectedly called.");
    qApp->exit(1);
}

void DeviceManagerTest::handleUpdated()
{
    m_slotsCalled << SlotUpdated;
}
