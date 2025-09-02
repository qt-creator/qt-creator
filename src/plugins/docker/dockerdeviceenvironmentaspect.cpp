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
    : Utils::TypedAspect<QStringList>(parent)
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
    envWidget->setUserChanges(EnvironmentItem::fromStringList(undoable.get()));

    connect(
        this, &DockerDeviceEnvironmentAspect::remoteEnvironmentChanged, envWidget, [this, envWidget] {
            if (m_remoteEnvironment)
                envWidget->setBaseEnvironment(*m_remoteEnvironment);
            else
                envWidget->setBaseEnvironment(Environment());
        });

    connect(&undoable.m_signal, &UndoSignaller::changed, envWidget, [this, envWidget] {
        if (EnvironmentItem::toStringList(envWidget->userChanges()) != undoable.get()) {
            envWidget->setUserChanges(EnvironmentItem::fromStringList(undoable.get()));
            handleGuiChanged();
        }
    });

    connect(envWidget, &EnvironmentWidget::userChangesChanged, this, [this, envWidget] {
        undoable.set(undoStack(), EnvironmentItem::toStringList(envWidget->userChanges()));
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
    result.modify(EnvironmentItem::fromStringList(value()));
    return result;
}
bool DockerDeviceEnvironmentAspect::guiToBuffer()
{
    const QStringList newValue = undoable.get();
    if (newValue != m_buffer) {
        m_buffer = newValue;
        return true;
    }
    return false;
}
void DockerDeviceEnvironmentAspect::bufferToGui()
{
    undoable.setWithoutUndo(m_buffer);
}

void DockerDeviceEnvironmentAspect::fromMap(const Utils::Store &map)
{
    if (skipSave())
        return;

    Store subMap = storeFromVariant(map.value(settingsKey()));

    if (subMap.contains(DEVICE_ENVIRONMENT_KEY)) {
        const QStringList deviceEnv = subMap.value(DEVICE_ENVIRONMENT_KEY).toStringList();
        NameValueDictionary envDict;
        for (const QString &env : deviceEnv) {
            const auto parts = env.split(QLatin1Char('='));
            if (parts.size() == 2)
                envDict.set(parts[0], parts[1]);
        }
        m_remoteEnvironment = Environment(envDict);
    }

    if (subMap.contains(CHANGES_KEY)) {
        const QStringList changes = subMap.value(CHANGES_KEY).toStringList();
        setValue(changes, BeQuiet);
    }
}
void DockerDeviceEnvironmentAspect::toMap(Utils::Store &map) const
{
    Store subMap;
    saveToMap(subMap, value(), defaultValue(), CHANGES_KEY);
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
