// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevice.h"

#include "../kitaspect.h"

#include <projectexplorer/projectexplorer_export.h>

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class ProjectExplorerPlugin;

class ToolDetectionRequest
{
public:
    IDevicePtr device;
    quint64 token = 0;
    Utils::FilePaths searchPaths;
    DetectionSource source;
    ToolDetectionLogger logger;
};

using ToolDetector = std::function<void(const ToolDetectionRequest &request,
                                        const std::function<void()> &done)>;

namespace Internal {
class DeviceManagerPrivate;
class DeviceSettingsWidget;

QObject *createDeviceManagerTest();
} // namespace Internal

class PROJECTEXPLORER_EXPORT DeviceManager final : public QObject
{
    Q_OBJECT
    friend class Internal::DeviceSettingsWidget;
    friend class IDevice;

public:
    DeviceManager();
    ~DeviceManager() final;

    static DeviceManager *instance();

    static int deviceCount();
    static IDevice::Ptr deviceAt(int index);

    static void forEachDevice(const std::function<void(const IDeviceConstPtr &)> &);

    static IDevice::Ptr find(Utils::Id id);
    static IDevice::Ptr defaultDevice(Utils::Id deviceType);
    static bool hasDevice(const QString &name);
    static bool isUnrestored(Utils::Id id);

    static void addDevice(const IDevice::Ptr &device);
    static void removeDevice(Utils::Id id);
    static void setDeviceState(
        Utils::Id deviceId, IDevice::DeviceState deviceState, bool announce = true);
    static IDevice::DeviceState deviceState(Utils::Id deviceId);
    static void announceDeviceUpdate(Utils::Id deviceId);

    static bool isLoaded();

    static IDevice::ConstPtr deviceForPath(const Utils::FilePath &path);
    static IDevice::ConstPtr defaultDesktopDevice();

    static void addToolDetector(QObject *guard, const QString &searchMessage,
                                const ToolDetector &detector);

signals:
    void deviceAdded(Utils::Id id);
    void deviceAboutToBeRemoved(Utils::Id id);
    void deviceRemoved(Utils::Id id);
    void deviceUpdated(Utils::Id id);
    void updated(); // Emitted for all of the above.

    void toolDetectionRequested(Utils::Id id, const Utils::FilePaths &searchPaths, quint64 token,
                                 const ToolDetectionLogger &logger);

    void devicesLoaded(); // Emitted once load() is done

private:
    static void load();
    static void save();
    static QList<IDevice::Ptr> fromMap(const Utils::Store &map,
                                       QHash<Utils::Id, Utils::Id> *defaultDevices,
                                       QList<Utils::Store> *unrestoredDevices = nullptr);
    static Utils::Store toMap();
    static void deviceFactoryAdded();
    static void restoreDevicesWithUnknownType();
    static void deviceToolFactoryAdded();

    // For SettingsWidget.
    static IDevice::Ptr mutableDevice(Utils::Id id);
    static void setDefaultDevice(Utils::Id id);

    friend class Internal::DeviceManagerPrivate;
    friend class DeviceToolAspectFactory;
    friend class IDeviceFactory;
    friend class ProjectExplorerPlugin;
    friend class ProjectExplorerPluginPrivate;
};

} // namespace ProjectExplorer
