/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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


#include "maemorunconfiguration.h"
#include "qt4project.h"
#include "qt4maemotarget.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/customexecutablerunconfiguration.h>
#include <QtGui/QApplication>
#include <QtGui/QIcon>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4MaemoTarget::Qt4MaemoTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new Qt4MaemoDeployConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName());
    setIcon(QIcon(":/projectexplorer/images/MaemoDevice.png"));
}

Qt4MaemoTarget::~Qt4MaemoTarget()
{

}

QString Qt4MaemoTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Maemo", "Qt4 Maemo target display name");
}

Qt4BuildConfigurationFactory *Qt4MaemoTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *Qt4MaemoTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

QString Qt4MaemoTarget::defaultBuildDirectory() const
{
    //TODO why?
#if defined(Q_OS_WIN)
    return project()->projectDirectory();
#endif
    return Qt4BaseTarget::defaultBuildDirectory();
}

void Qt4MaemoTarget::createApplicationProFiles()
{
    removeUnconfiguredCustomExectutableRunConfigurations();

    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (MaemoRunConfiguration *qt4rc = qobject_cast<MaemoRunConfiguration *>(rc))
            paths.remove(qt4rc->proFilePath());

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, paths)
        addRunConfiguration(new MaemoRunConfiguration(this, path));

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    }
}
