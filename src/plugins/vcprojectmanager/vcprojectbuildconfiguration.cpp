/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "vcmakestep.h"
#include "vcprojectbuildconfiguration.h"
#include "vcprojectmanagerconstants.h"

#include <coreplugin/mimedatabase.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace VcProjectManager::Constants;

////////////////////////////////////
// VcProjectBuildConfiguration class
////////////////////////////////////
namespace VcProjectManager {
namespace Internal {

VcProjectBuildConfiguration::VcProjectBuildConfiguration(Target *parent) :
    BuildConfiguration(parent, Core::Id(VC_PROJECT_BC_ID))
{
    m_buildDirectory = static_cast<VcProject *>(parent->project())->defaultBuildDirectory();
}

VcProjectBuildConfiguration::~VcProjectBuildConfiguration()
{
}

NamedWidget *VcProjectBuildConfiguration::createConfigWidget()
{
    return new VcProjectBuildSettingsWidget;
}

QString VcProjectBuildConfiguration::buildDirectory() const
{
    return m_buildDirectory;
}

BuildConfiguration::BuildType VcProjectBuildConfiguration::buildType() const
{
    return Debug;
}

void VcProjectBuildConfiguration::setConfiguration(IConfiguration *config)
{
    m_configuration = config;
    connect(m_configuration, SIGNAL(nameChanged()), this, SLOT(reloadConfigurationName()));
}

QVariantMap VcProjectBuildConfiguration::toMap() const
{
    return BuildConfiguration::toMap();
}

void VcProjectBuildConfiguration::reloadConfigurationName()
{
    setDisplayName(m_configuration->fullName());
    setDefaultDisplayName(m_configuration->fullName());
}

VcProjectBuildConfiguration::VcProjectBuildConfiguration(Target *parent, VcProjectBuildConfiguration *source)
    : BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

bool VcProjectBuildConfiguration::fromMap(const QVariantMap &map)
{
    return BuildConfiguration::fromMap(map);
}

///////////////////////////////////////////
// VcProjectBuildConfigurationFactory class
///////////////////////////////////////////
VcProjectBuildConfigurationFactory::VcProjectBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{
}

int VcProjectBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

int VcProjectBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    return (k && Core::MimeDatabase::findByFile(QFileInfo(projectPath))
            .matchesType(QLatin1String(Constants::VCPROJ_MIMETYPE))) ? 0 : -1;
}

QList<BuildInfo *> VcProjectBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;
    result << createBuildInfo(parent->kit(),
                              parent->project()->projectDirectory());
    return result;
}

QList<BuildInfo *> VcProjectBuildConfigurationFactory::availableSetups(
        const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;
    // TODO: Populate from Configuration
    BuildInfo *info = createBuildInfo(k,
                                      Utils::FileName::fromString(VcProject::defaultBuildDirectory(projectPath)));
    info->displayName = tr("Default");
    result << info;
    return result;
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(parent, return 0);
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    bc->setBuildDirectory(info->buildDirectory);

    return bc;
}

bool VcProjectBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    return canHandle(parent) && source->id() == VC_PROJECT_BC_ID;
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    VcProjectBuildConfiguration *old = static_cast<VcProjectBuildConfiguration *>(source);
    return new VcProjectBuildConfiguration(parent, old);
}

bool VcProjectBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    return canHandle(parent) && idFromMap(map) == VC_PROJECT_BC_ID;
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool VcProjectBuildConfigurationFactory::canHandle(const Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<VcProject *>(t->project());
}

BuildInfo *VcProjectBuildConfigurationFactory::createBuildInfo(const ProjectExplorer::Kit *k,
                                                               const Utils::FileName &buildDir) const
{
    BuildInfo *info = new BuildInfo(this);
    info->typeName = tr("Build");
    info->buildDirectory = buildDir;
    info->kitId = k->id();
    info->supportsShadowBuild = true;

    return info;
}

} // namespace Internal
} // namespace VcProjectManager
