// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "docker_global.h"

#include <projectexplorer/environmentaspect.h>

namespace Docker {

class DOCKER_EXPORT DockerDeviceEnvironmentAspect : public ProjectExplorer::EnvironmentAspect
{
    Q_OBJECT

public:
    explicit DockerDeviceEnvironmentAspect(Utils::AspectContainer *container = nullptr);

    void setRemoteEnvironment(const Utils::Environment &env);

    QString userEnvironmentChangesAsString() const;

    Utils::Environment operator()() const { return environment(); }

    bool isRemoteEnvironmentSet() const { return m_remoteEnvironment.hasChanges(); }

protected:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

private:
    Utils::Environment m_remoteEnvironment;
};

} // namespace Docker
