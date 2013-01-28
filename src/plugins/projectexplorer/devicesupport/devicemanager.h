/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "idevice.h"

#include <projectexplorer/projectexplorer_export.h>
#include <utils/fileutils.h>

#include <utils/fileutils.h>

#include <QObject>

namespace ProjectExplorer {
class IDevice;
class IDeviceFactory;

namespace Internal {
class DeviceManagerPrivate;
class DeviceSettingsWidget;
} // namespace Internal

class PROJECTEXPLORER_EXPORT DeviceManager : public QObject
{
    Q_OBJECT
    friend class Internal::DeviceSettingsWidget;
    friend class IDevice;

public:
    ~DeviceManager();

    static DeviceManager *instance();

    int deviceCount() const;
    IDevice::ConstPtr deviceAt(int index) const;

    IDevice::ConstPtr find(Core::Id id) const;
    IDevice::ConstPtr findInactiveAutoDetectedDevice(Core::Id type, Core::Id id);
    IDevice::ConstPtr defaultDevice(Core::Id deviceType) const;
    bool hasDevice(const QString &name) const;
    Core::Id deviceId(const IDevice::ConstPtr &device) const;

    void addDevice(const IDevice::Ptr &device);
    void removeDevice(Core::Id id);

signals:
    void deviceAdded(Core::Id id);
    void deviceRemoved(Core::Id id);
    void deviceUpdated(Core::Id id);
    void deviceListChanged();
    void updated(); // Emitted for all of the above.

private slots:
    void save();

private:
    DeviceManager(bool isInstance = false);

    void load();
    void loadPre2_6();
    static const IDeviceFactory *restoreFactory(const QVariantMap &map);
    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;
    void ensureOneDefaultDevicePerType();

    // For SettingsWidget.
    IDevice::Ptr mutableDevice(Core::Id id) const;
    void setDefaultDevice(int index);
    static DeviceManager *cloneInstance();
    static void replaceInstance();
    static void removeClonedInstance();

    // For IDevice.
    IDevice::Ptr fromRawPointer(IDevice *device) const;
    IDevice::ConstPtr fromRawPointer(const IDevice *device) const;

    static Utils::FileName settingsFilePath(const QString &extension);
    static void copy(const DeviceManager *source, DeviceManager *target, bool deep);

    Internal::DeviceManagerPrivate * const d;
};

} // namespace ProjectExplorer

#endif // DEVICEMANAGER_H
