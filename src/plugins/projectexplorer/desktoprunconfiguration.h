/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "runconfigurationaspects.h"
#include "runcontrol.h"

namespace ProjectExplorer {
namespace Internal {

class DesktopRunConfiguration : public RunConfiguration
{
    Q_OBJECT

protected:
    enum Kind { Qmake, Qbs, CMake }; // FIXME: Remove

    DesktopRunConfiguration(Target *target, Core::Id id, Kind kind);

private:
    void doAdditionalSetup(const RunConfigurationCreationInfo &info) final;
    bool fromMap(const QVariantMap &map) final;
    void updateEnabledState() final;

    void updateTargetInformation();

    Utils::FilePath executableToRun(const BuildTargetInfo &targetInfo) const;
    QString disabledReason() const override;

    bool isBuildTargetValid() const;

    const Kind m_kind;
};

class DesktopQmakeRunConfigurationFactory : public RunConfigurationFactory
{
public:
    DesktopQmakeRunConfigurationFactory();
};

class QbsRunConfigurationFactory : public RunConfigurationFactory
{
public:
    QbsRunConfigurationFactory();
};

class CMakeRunConfigurationFactory : public RunConfigurationFactory
{
public:
    CMakeRunConfigurationFactory();
};

} // namespace Internal
} // namespace ProjectExplorer
