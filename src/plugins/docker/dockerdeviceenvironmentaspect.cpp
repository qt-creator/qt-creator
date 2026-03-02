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
#include <QStandardItem>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker {

const char DEVICE_ENVIRONMENT_KEY[] = "RemoteEnvironment";
const char CHANGES_KEY[] = "UserChanges";

DockerDeviceEnvironmentAspect::DockerDeviceEnvironmentAspect(Utils::AspectContainer *parent)
    : EnvironmentChangesAspect(parent)
{}

void DockerDeviceEnvironmentAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    undoable.setSilently(value());

    using namespace Layouting;

    // clang-format off
    auto fetchBtn = Row {
        st,
        PushButton {
            text(Tr::tr("Fetch Environment")),
            onClicked(this, [this]{ emit fetchRequested(); })
        },
        st
    }.emerge();
    // clang-format on

    auto envWidget = new EnvironmentWidget(nullptr, EnvironmentWidget::Type::TypeRemote, fetchBtn);
    envWidget->setOpenTerminalFunc(nullptr);
    envWidget->setChanges(undoable.get());
    envWidget->setupDirtyHooks();

    connect(
        this, &DockerDeviceEnvironmentAspect::remoteEnvironmentChanged, envWidget, [this, envWidget] {
            if (m_remoteEnvironment)
                envWidget->setBaseEnvironment(*m_remoteEnvironment);
            else
                envWidget->setBaseEnvironment(Environment());
        });

    connect(&undoable.m_signal, &UndoSignaller::changed, envWidget, [this, envWidget] {
        if (envWidget->changes() != undoable.get()) {
            envWidget->setChanges(undoable.get());
            handleGuiChanged();
        }
    });

    connect(envWidget, &EnvironmentWidget::userChangesChanged, this, [this, envWidget] {
        undoable.set(undoStack(), envWidget->changes());
        handleGuiChanged();
    });

    if (m_remoteEnvironment)
        envWidget->setBaseEnvironment(*m_remoteEnvironment);

    registerSubWidget(envWidget);
    addLabeledItem(parent, envWidget);
}

Utils::Environment DockerDeviceEnvironmentAspect::operator()() const
{
    Environment result = m_remoteEnvironment.value_or(Environment());
    value().modifyEnvironment(result, macroExpander());
    return result;
}
bool DockerDeviceEnvironmentAspect::guiToVolatileValue()
{
    const EnvironmentChanges newValue = undoable.get();
    if (newValue != m_volatileValue) {
        m_volatileValue = newValue;
        return true;
    }
    return false;
}
void DockerDeviceEnvironmentAspect::volatileValueToGui()
{
    undoable.setWithoutUndo(m_volatileValue);
}

void DockerDeviceEnvironmentAspect::fromMap(const Utils::Store &map)
{
    if (skipSave())
        return;

    Store subMap = storeFromVariant(map.value(settingsKey()));

    if (subMap.contains(DEVICE_ENVIRONMENT_KEY)) {
        const QStringList deviceEnv = subMap.value(DEVICE_ENVIRONMENT_KEY).toStringList();
        OsType osType = HostOsInfo::hostOs();
        if (IDevice *device = dynamic_cast<IDevice *>(container()))
            osType = device->osType();
        m_remoteEnvironment = Environment(NameValueDictionary(deviceEnv, osType));
    }

    if (subMap.contains(CHANGES_KEY)) {
        const auto changes = EnvironmentChanges::createFromVariant(
            subMap.value(CHANGES_KEY));
        setValue(changes, BeQuiet);
    }
}
void DockerDeviceEnvironmentAspect::toMap(Utils::Store &map) const
{
    Store subMap;
    saveToMap(subMap, value().toVariant(), defaultValue().toVariant(), CHANGES_KEY);
    if (m_remoteEnvironment.has_value()) {
        subMap.insert(DEVICE_ENVIRONMENT_KEY, m_remoteEnvironment->toStringList());
    }

    saveToMap(map, mapFromStore(subMap), QVariant(), settingsKey());
}

void DockerDeviceEnvironmentAspect::setRemoteEnvironment(const Utils::Environment &env)
{
    m_remoteEnvironment = env;
    emit remoteEnvironmentChanged();
}
} // namespace Docker
