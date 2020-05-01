/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "mesonprojectparser.h"
#include <exewrappers/mesonwrapper.h>
#include <kithelper/kitdata.h>

#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/target.h>
#include <utils/filesystemwatcher.h>
#include <QObject>

namespace MesonProjectManager {
namespace Internal {
class MesonBuildConfiguration;
class MesonBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT
public:
    MesonBuildSystem(MesonBuildConfiguration *bc);
    ~MesonBuildSystem() final;

    void triggerParsing() final;

    inline const BuildOptionsList &buildOptions() const { return m_parser.buildOptions(); }
    inline const TargetsList &targets() const { return m_parser.targets(); }

    bool configure();
    bool setup();
    bool wipe();

    MesonBuildConfiguration *mesonBuildConfiguration();

    const QStringList &targetList() const { return m_parser.targetsNames(); }

    void setMesonConfigArgs(const QStringList &args) { m_pendingConfigArgs = args; }

private:
    void init();
    bool parseProject();
    void updateKit(ProjectExplorer::Kit *kit);
    bool needsSetup();
    void parsingCompleted(bool success);
    ProjectExplorer::Kit *kit();
    QStringList configArgs(bool isSetup);
    ProjectExplorer::BuildSystem::ParseGuard m_parseGuard;
    MesonProjectParser m_parser;
    CppTools::CppProjectUpdater m_cppCodeModelUpdater;
    QStringList m_pendingConfigArgs;
    KitData m_kitData;
};
} // namespace Internal
} // namespace MesonProjectManager
