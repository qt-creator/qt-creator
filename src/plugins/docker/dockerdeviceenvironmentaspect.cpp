// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdeviceenvironmentaspect.h"

#include "dockertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspectwidget.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker {

const char DISPLAY_KEY[] = "DISPLAY";
const char VERSION_KEY[] = "Docker.EnvironmentAspect.Version";
const char DEVICE_ENVIRONMENT_KEY[] = "Docker.DeviceEnvironment";
const int ENVIRONMENTASPECT_VERSION = 1; // Version was introduced in 4.3 with the value 1

class DockerDeviceEnvironmentAspectWidget : public EnvironmentAspectWidget
{
public:
    DockerDeviceEnvironmentAspectWidget(DockerDeviceEnvironmentAspect *aspect)
        : EnvironmentAspectWidget(aspect)
    {
        //auto fetchButton = new QPushButton(Tr::tr("Fetch Device Environment"));
        //addWidget(fetchButton);
        //
        //connect(fetchButton, &QPushButton::clicked, this, [aspect] {
        //    if (IDevice::ConstPtr device = RunDeviceKitAspect::device(aspect->kit())) {
        //        DeviceFileAccess *access = device->fileAccess();
        //        QTC_ASSERT(access, return);
        //        aspect->setRemoteEnvironment(access->deviceEnvironment());
        //    }
        //});
    }
};

static bool displayAlreadySet(const Utils::EnvironmentItems &changes)
{
    return Utils::contains(changes, [](const Utils::EnvironmentItem &item) {
        return item.name == DISPLAY_KEY;
    });
}

DockerDeviceEnvironmentAspect::DockerDeviceEnvironmentAspect(AspectContainer *container)
    : EnvironmentAspect(container)
{
    setAllowPrintOnRun(false);

    addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});
    addPreferredBaseEnvironment(Tr::tr("Device Environment"), [this] {
        return m_remoteEnvironment;
    });

    setConfigWidgetCreator([this] { return new DockerDeviceEnvironmentAspectWidget(this); });
}

void DockerDeviceEnvironmentAspect::setRemoteEnvironment(const Utils::Environment &env)
{
    if (env != m_remoteEnvironment) {
        m_remoteEnvironment = env;
        setBaseEnvironmentBase(1);
        emit environmentChanged();
    }
}

QString DockerDeviceEnvironmentAspect::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    const Utils::EnvironmentItems items = userEnvironmentChanges();
    for (const Utils::EnvironmentItem &item : items)
        env.append(placeHolder.arg(item.name, item.value));
    return env.mid(0, env.size() - 1);
}

void DockerDeviceEnvironmentAspect::fromMap(const Store &map)
{
    ProjectExplorer::EnvironmentAspect::fromMap(map);

    const auto version = map.value(VERSION_KEY, 0).toInt();
    if (version == 0) {
        // In Qt Creator versions prior to 4.3 RemoteLinux included DISPLAY=:0.0 in the base
        // environment, if DISPLAY was not set. In order to keep existing projects expecting
        // that working, add the DISPLAY setting to user changes in them. New projects will
        // have version 1 and will not get DISPLAY set.
        auto changes = userEnvironmentChanges();
        if (!displayAlreadySet(changes)) {
            changes.append(
                Utils::EnvironmentItem(QLatin1String(DISPLAY_KEY), QLatin1String(":0.0")));
            setUserEnvironmentChanges(changes);
        }
    }

    if (map.contains(DEVICE_ENVIRONMENT_KEY)) {
        const QStringList deviceEnv = map.value(DEVICE_ENVIRONMENT_KEY).toStringList();
        NameValueDictionary envDict;
        for (const QString &env : deviceEnv) {
            const auto parts = env.split(QLatin1Char('='));
            if (parts.size() == 2)
                envDict.set(parts[0], parts[1]);
        }
        m_remoteEnvironment = Environment(envDict);
        if (baseEnvironmentBase() == -1)
            setBaseEnvironmentBase(1);
    }
}

void DockerDeviceEnvironmentAspect::toMap(Store &map) const
{
    ProjectExplorer::EnvironmentAspect::toMap(map);
    map.insert(VERSION_KEY, ENVIRONMENTASPECT_VERSION);
    map.insert(DEVICE_ENVIRONMENT_KEY, m_remoteEnvironment.toStringList());
}

} // namespace Docker
