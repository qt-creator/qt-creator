/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"

#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using ProjectExplorer::BuildConfiguration;

GenericBuildConfiguration::GenericBuildConfiguration(GenericProject *pro)
    : BuildConfiguration(pro)
{

}

GenericBuildConfiguration::GenericBuildConfiguration(GenericProject *pro, const QVariantMap &map)
    : BuildConfiguration(pro, map)
{
    m_buildDirectory = map.value("buildDirectory").toString();
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericBuildConfiguration *source)
    : BuildConfiguration(source),
    m_buildDirectory(source->m_buildDirectory)
{

}

void GenericBuildConfiguration::toMap(QVariantMap &map) const
{
    map.insert("buildDirectory", m_buildDirectory);
}

ProjectExplorer::Environment GenericBuildConfiguration::environment() const
{
    return ProjectExplorer::Environment::systemEnvironment();
}

QString GenericBuildConfiguration::buildDirectory() const
{
    QString buildDirectory = m_buildDirectory;
    if (buildDirectory.isEmpty()) {
        QFileInfo fileInfo(project()->file()->fileName());

        buildDirectory = fileInfo.absolutePath();
    }
    return buildDirectory;
}

void GenericBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
}

GenericProject *GenericBuildConfiguration::genericProject() const
{
    return static_cast<GenericProject *>(project());
}


/*!
  \class GenericBuildConfigurationFactory
*/

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory(GenericProject *project)
    : IBuildConfigurationFactory(project),
    m_project(project)
{
}

GenericBuildConfigurationFactory::~GenericBuildConfigurationFactory()
{
}

QStringList GenericBuildConfigurationFactory::availableCreationIds() const
{
    return QStringList() << "Create";
}

QString GenericBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    QTC_ASSERT(id == "Create", return QString());
    return tr("Create");
}

BuildConfiguration *GenericBuildConfigurationFactory::create(const QString &id) const
{
    QTC_ASSERT(id == "Create", return false);
    //TODO asking for name is duplicated everywhere, but maybe more
    // wizards will show up, that incorporate choosing the name
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(m_project);
    bc->setDisplayName(buildConfigurationName);
    m_project->addBuildConfiguration(bc); // also makes the name unique...

    GenericMakeStep *makeStep = new GenericMakeStep(bc);
    bc->insertBuildStep(0, makeStep);
    makeStep->setBuildTarget("all", /* on = */ true);
    return bc;
}

BuildConfiguration *GenericBuildConfigurationFactory::clone(BuildConfiguration *source) const
{
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(static_cast<GenericBuildConfiguration *>(source));
    return bc;
}

BuildConfiguration *GenericBuildConfigurationFactory::restore(const QVariantMap &map) const
{
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(m_project, map);
    return bc;
}
