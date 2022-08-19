// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/environmentaspectwidget.h>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace RemoteLinux {

class RemoteLinuxEnvironmentAspect;

namespace Internal { class RemoteLinuxEnvironmentReader; }

class RemoteLinuxEnvironmentAspectWidget : public ProjectExplorer::EnvironmentAspectWidget
{
    Q_OBJECT

public:
    RemoteLinuxEnvironmentAspectWidget(RemoteLinuxEnvironmentAspect *aspect,
                                       ProjectExplorer::Target *target);

private:
    void fetchEnvironment();
    void fetchEnvironmentFinished();
    void fetchEnvironmentError(const QString &error);
    void stopFetchEnvironment();

    Internal::RemoteLinuxEnvironmentReader *m_deviceEnvReader = nullptr;
    QPushButton *m_fetchButton = nullptr;
};

} // namespace RemoteLinux
