// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "docker_global.h"

#include <projectexplorer/environmentaspect.h>

namespace Docker {

class DOCKER_EXPORT DockerDeviceEnvironmentAspect : public Utils::TypedAspect<QStringList>
{
    Q_OBJECT
public:
    DockerDeviceEnvironmentAspect(Utils::AspectContainer *parent);

    void addToLayoutImpl(Layouting::Layout &parent) override;

    void setRemoteEnvironment(const Utils::Environment &env);
    bool isRemoteEnvironmentSet() const { return m_remoteEnvironment.has_value(); }

    bool guiToBuffer() override;
    void bufferToGui() override;

    Utils::Environment operator()() const;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

signals:
    void fetchRequested();
    void remoteEnvironmentChanged();

protected:
    std::optional<Utils::Environment> m_remoteEnvironment;
    Utils::UndoableValue<QStringList> undoable;
};

} // namespace Docker
