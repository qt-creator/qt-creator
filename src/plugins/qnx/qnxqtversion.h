// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qnxconstants.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

#include <utils/environment.h>

namespace Qnx::Internal {

class QnxQtVersion : public QtSupport::QtVersion
{
public:
    QnxQtVersion();

    QString description() const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;

    QString qnxHost() const;
    Utils::FilePath qnxTarget() const;

    QString cpuDir() const;

    Utils::Store toMap() const override;
    void fromMap(const Utils::Store &map,
                 const Utils::FilePath &filePath,
                 bool forceRefreshCache) override;

    ProjectExplorer::Abis detectQtAbis() const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    void setupQmakeRunEnvironment(Utils::Environment &env) const override;

    QtSupport::QtConfigWidget *createConfigurationWidget() const override;

    bool isValid() const override;
    QString invalidReason() const override;

    Utils::FilePath sdpPath() const;
    void setSdpPath(const Utils::FilePath &sdpPath);

    Utils::EnvironmentItems environment() const;

private:
    void updateEnvironment() const;

    Utils::FilePath m_sdpPath;

    mutable QString m_cpuDir;
    mutable bool m_environmentUpToDate = false;
    mutable Utils::EnvironmentItems m_qnxEnv;
};

class QnxQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    QnxQtVersionFactory();
};

} // Qnx::Internal
