// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "abstractremotelinuxdeployservice.h"

#include <QList>

namespace ProjectExplorer { class DeployableFile; }

namespace RemoteLinux {
namespace Internal { class GenericDirectUploadServicePrivate; }

enum class IncrementalDeployment { Enabled, Disabled, NotSupported };

class REMOTELINUX_EXPORT GenericDirectUploadService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    GenericDirectUploadService(QObject *parent = nullptr);
    ~GenericDirectUploadService();

    void setDeployableFiles(const QList<ProjectExplorer::DeployableFile> &deployableFiles);
    void setIncrementalDeployment(IncrementalDeployment incremental);
    void setIgnoreMissingFiles(bool ignoreMissingFiles);

private:
    bool isDeploymentNecessary() const final;
    Utils::Tasking::Group deployRecipe() final;

    friend class Internal::GenericDirectUploadServicePrivate;
    Internal::GenericDirectUploadServicePrivate * const d;
};

} //namespace RemoteLinux
