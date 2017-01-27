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

namespace QmakeProjectManager {
class QmakeProFileNode;
}

namespace Ios {
namespace Internal {

class IosDeployStep;
class IosRunConfigurationFactory;
class IosRunConfigurationWidget;

class IosRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class IosRunConfigurationFactory;

public:
    IosRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const Utils::FileName &path);

    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;
    IosDeployStep *deployStep() const;

    QString commandLineArguments() const;
    Utils::FileName profilePath() const;
    QString applicationName() const;
    Utils::FileName bundleDirectory() const;
    Utils::FileName localExecutable() const;
    bool isEnabled() const override;
    QString disabledReason() const override;
    IosDeviceType deviceType() const;
    void setDeviceType(const IosDeviceType &deviceType);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    QString buildSystemTarget() const final;

protected:
    IosRunConfiguration(ProjectExplorer::Target *parent, IosRunConfiguration *source);

signals:
    void localExecutableChanged();

private:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *pro, bool success, bool parseInProgress);
    void deviceChanges();
    void init();
    void enabledCheck();
    friend class IosRunConfigurationWidget;
    void updateDisplayNames();

    Utils::FileName m_profilePath;
    QString m_lastDisabledReason;
    bool m_lastIsEnabled;
    bool m_parseInProgress;
    bool m_parseSuccess;
    IosDeviceType m_deviceType;
};

} // namespace Internal
} // namespace Ios
