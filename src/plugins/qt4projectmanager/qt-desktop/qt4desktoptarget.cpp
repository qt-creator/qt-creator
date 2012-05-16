/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4desktoptarget.h"
#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4runconfiguration.h"
#include "qt4buildconfiguration.h"
#include <projectexplorer/deployconfiguration.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <QApplication>
#include <QStyle>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4DesktopTarget::Qt4DesktopTarget(Qt4Project *parent, const Core::Id id) :
    Qt4BaseTarget(parent, id),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName());
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

Qt4DesktopTarget::~Qt4DesktopTarget()
{
}

QString Qt4DesktopTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Desktop", "Qt4 Desktop target display name");
}

ProjectExplorer::IBuildConfigurationFactory *Qt4DesktopTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void Qt4DesktopTarget::createApplicationProFiles(bool reparse)
{
    if (!reparse)
        removeUnconfiguredCustomExectutableRunConfigurations();

    // We use the list twice
    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QHash<QString, bool> paths;
    foreach (Qt4ProFileNode *pro, profiles) {
        bool isConsole = pro->variableValue(ConfigVar).contains(QLatin1String("console"));
        paths.insert(pro->path(), isConsole);
    }

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (Qt4RunConfiguration *qt4rc = qobject_cast<Qt4RunConfiguration *>(rc)) {
            paths.remove(qt4rc->proFilePath());
        }

    // Only add new runconfigurations if there are none.
    QHash<QString, bool>::const_iterator it, end;
    end = paths.constEnd();
    for (it = paths.constBegin(); it != end; ++it) {
        Qt4RunConfiguration *qt4rc = new Qt4RunConfiguration(this, it.key());
        if (it.value())
            qt4rc->setRunMode(ProjectExplorer::LocalApplicationRunConfiguration::Console);
        addRunConfiguration(qt4rc);
    }

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> Qt4DesktopTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (Qt4RunConfiguration *qt4c = qobject_cast<Qt4RunConfiguration *>(rc))
            if (qt4c->proFilePath() == n->path())
                result << rc;
    return result;
}
