// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxenvironmentaspect.h"

#include "linuxdevice.h"
#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/environmentaspectwidget.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const char DISPLAY_KEY[] = "DISPLAY";
const char VERSION_KEY[] = "RemoteLinux.EnvironmentAspect.Version";
const int ENVIRONMENTASPECT_VERSION = 1; // Version was introduced in 4.3 with the value 1

class RemoteLinuxEnvironmentAspectWidget : public EnvironmentAspectWidget
{
public:
    RemoteLinuxEnvironmentAspectWidget(RemoteLinuxEnvironmentAspect *aspect)
        : EnvironmentAspectWidget(aspect)
    {
        auto fetchButton = new QPushButton(Tr::tr("Fetch Device Environment"));
        addWidget(fetchButton);

        connect(aspect->target(), &Target::kitChanged, aspect, [aspect] {
            aspect->setRemoteEnvironment({});
        });

        connect(fetchButton, &QPushButton::clicked, this, [aspect] {
            if (IDevice::ConstPtr device = DeviceKitAspect::device(aspect->target()->kit())) {
                DeviceFileAccess *access = device->fileAccess();
                QTC_ASSERT(access, return);
                aspect->setRemoteEnvironment(access->deviceEnvironment());
            }
        });

        envWidget()->setOpenTerminalFunc([aspect](const Environment &env) {
            IDevice::ConstPtr device = DeviceKitAspect::device(aspect->target()->kit());
            if (!device) {
                QMessageBox::critical(Core::ICore::dialogParent(),
                                      Tr::tr("Cannot Open Terminal"),
                                      Tr::tr("Cannot open remote terminal: Current kit has no device."));
                return;
            }
            const auto linuxDevice = device.dynamicCast<const LinuxDevice>();
            QTC_ASSERT(linuxDevice, return);
            linuxDevice->openTerminal(env, FilePath());
        });
    }
};

static bool displayAlreadySet(const Utils::EnvironmentItems &changes)
{
    return Utils::contains(changes, [](const Utils::EnvironmentItem &item) {
        return item.name == DISPLAY_KEY;
    });
}

RemoteLinuxEnvironmentAspect::RemoteLinuxEnvironmentAspect(AspectContainer *container)
    : EnvironmentAspect(container)
{
    addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});
    addPreferredBaseEnvironment(Tr::tr("System Environment"), [this] { return m_remoteEnvironment; });

    setConfigWidgetCreator([this] { return new RemoteLinuxEnvironmentAspectWidget(this); });
}

void RemoteLinuxEnvironmentAspect::setRemoteEnvironment(const Utils::Environment &env)
{
    if (env != m_remoteEnvironment) {
        m_remoteEnvironment = env;
        emit environmentChanged();
    }
}

QString RemoteLinuxEnvironmentAspect::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    const Utils::EnvironmentItems items = userEnvironmentChanges();
    for (const Utils::EnvironmentItem &item : items)
        env.append(placeHolder.arg(item.name, item.value));
    return env.mid(0, env.size() - 1);
}

void RemoteLinuxEnvironmentAspect::fromMap(const Store &map)
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
            changes.append(Utils::EnvironmentItem(QLatin1String(DISPLAY_KEY), QLatin1String(":0.0")));
            setUserEnvironmentChanges(changes);
        }
    }
}

void RemoteLinuxEnvironmentAspect::toMap(Store &map) const
{
    ProjectExplorer::EnvironmentAspect::toMap(map);
    map.insert(VERSION_KEY, ENVIRONMENTASPECT_VERSION);
}

} // namespace RemoteLinux

