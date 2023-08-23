// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/environmentaspect.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT RemoteLinuxEnvironmentAspect : public ProjectExplorer::EnvironmentAspect
{
    Q_OBJECT

public:
    explicit RemoteLinuxEnvironmentAspect(Utils::AspectContainer *container = nullptr);

    void setRemoteEnvironment(const Utils::Environment &env);

    QString userEnvironmentChangesAsString() const;

protected:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

private:
    Utils::Environment m_remoteEnvironment;
};

} // namespace RemoteLinux
