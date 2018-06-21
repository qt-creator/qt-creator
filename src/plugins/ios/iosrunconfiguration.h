/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iossimulator.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/fileutils.h>

namespace Ios {
namespace Internal {

class IosDeployStep;
class IosRunConfigurationFactory;
class IosRunConfigurationWidget;

class IosRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    IosRunConfiguration(ProjectExplorer::Target *target, Core::Id id);

    QWidget *createConfigurationWidget() override;
    IosDeployStep *deployStep() const;

    Utils::FileName profilePath() const;
    QString applicationName() const;
    Utils::FileName bundleDirectory() const;
    Utils::FileName localExecutable() const;
    QString disabledReason() const override;
    IosDeviceType deviceType() const;
    void setDeviceType(const IosDeviceType &deviceType);

    void doAdditionalSetup(const ProjectExplorer::RunConfigurationCreationInfo &) override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

signals:
    void localExecutableChanged();

private:
    void deviceChanges();
    friend class IosRunConfigurationWidget;
    void updateDeviceType();
    void updateDisplayNames();
    void updateEnabledState() final;
    bool canRunForNode(const ProjectExplorer::Node *node) const final;

    IosDeviceType m_deviceType;
};

class IosRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    IosRunConfigurationFactory();
};

} // namespace Internal
} // namespace Ios
