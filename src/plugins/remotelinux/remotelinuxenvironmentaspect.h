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
    RemoteLinuxEnvironmentAspect(ProjectExplorer::Target *target);

    void setRemoteEnvironment(const Utils::Environment &env);

    QString userEnvironmentChangesAsString() const;

protected:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    Utils::Environment m_remoteEnvironment;
};

} // namespace RemoteLinux
