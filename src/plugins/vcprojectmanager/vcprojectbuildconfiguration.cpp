/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "vcprojectbuildconfiguration.h"
#include "vcprojectmanagerconstants.h"
#include "vcmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <projectexplorer/target.h>

#include <QFormLayout>
#include <QLabel>
#include <QInputDialog>

////////////////////////////////////
// VcProjectBuildConfiguration class
////////////////////////////////////
namespace VcProjectManager {
namespace Internal {

VcProjectBuildConfiguration::VcProjectBuildConfiguration(ProjectExplorer::Target *parent) :
    BuildConfiguration(parent, Core::Id(Constants::VC_PROJECT_BC_ID))
{
    m_buildDirectory = static_cast<VcProject *>(parent->project())->defaultBuildDirectory();
}

ProjectExplorer::NamedWidget *VcProjectBuildConfiguration::createConfigWidget()
{
    return new VcProjectBuildSettingsWidget;
}

QString VcProjectBuildConfiguration::buildDirectory() const
{
    return m_buildDirectory;
}

ProjectExplorer::IOutputParser *VcProjectBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::IOutputParser *parserchain = new ProjectExplorer::GnuMakeParser;

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc)
        parserchain->appendOutputParser(tc->outputParser());
    return parserchain;
}

ProjectExplorer::BuildConfiguration::BuildType VcProjectBuildConfiguration::buildType() const
{
    return Debug;
}

void VcProjectBuildConfiguration::setConfiguration(IConfiguration *config)
{
    m_configuration = config;
    connect(m_configuration, SIGNAL(nameChanged()), this, SLOT(reloadConfigurationName()));
}

QString VcProjectBuildConfiguration::configurationNameOnly() const
{
    QStringList splits = m_configuration->fullName().split(QLatin1Char('|'));

    if (splits.isEmpty())
        return QString();

    return splits[0];
}

QString VcProjectBuildConfiguration::platformNameOnly() const
{
    QStringList splits = m_configuration->fullName().split(QLatin1Char('|'));

    if (splits.isEmpty() || splits.size() <= 1 || splits.size() > 2)
        return QString();

    return splits[1];
}

QVariantMap VcProjectBuildConfiguration::toMap() const
{
    return ProjectExplorer::BuildConfiguration::toMap();
}

void VcProjectBuildConfiguration::reloadConfigurationName()
{
    setDisplayName(m_configuration->fullName());
    setDefaultDisplayName(m_configuration->fullName());
}

VcProjectBuildConfiguration::VcProjectBuildConfiguration(ProjectExplorer::Target *parent, VcProjectBuildConfiguration *source)
    : BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

bool VcProjectBuildConfiguration::fromMap(const QVariantMap &map)
{
    return ProjectExplorer::BuildConfiguration::fromMap(map);
}

///////////////////////////////////////////
// VcProjectBuildConfigurationFactory class
///////////////////////////////////////////
VcProjectBuildConfigurationFactory::VcProjectBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{
}

QList<Core::Id> VcProjectBuildConfigurationFactory::availableCreationIds(const ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    return QList<Core::Id>() << Core::Id(Constants::VC_PROJECT_BC_ID);
}

QString VcProjectBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::VC_PROJECT_BC_ID)
        return tr("Vc Project");

    return QString();
}

bool VcProjectBuildConfigurationFactory::canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (id == Constants::VC_PROJECT_BC_ID)
        return true;
    return false;
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;
//    VcProject *project = static_cast<VcProject *>(parent->project());

    bool ok = true;
    QString buildConfigName = name;
    if (buildConfigName.isEmpty())
        buildConfigName = QInputDialog::getText(0,
                                                tr("New Vc Project Configuration"),
                                                tr("New Configuration name:"),
                                                QLineEdit::Normal,
                                                QString(), &ok);
    buildConfigName = buildConfigName.trimmed();
    if (!ok || buildConfigName.isEmpty())
        return 0;

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    bc->setDisplayName(buildConfigName);
    bc->setDefaultDisplayName(buildConfigName);

    return bc;
}

bool VcProjectBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool VcProjectBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

VcProjectBuildConfiguration *VcProjectBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    VcProjectBuildConfiguration *old = static_cast<VcProjectBuildConfiguration *>(source);
    return new VcProjectBuildConfiguration(parent, old);
}

bool VcProjectBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<VcProject *>(t->project());
}

} // namespace Internal
} // namespace VcProjectManager
