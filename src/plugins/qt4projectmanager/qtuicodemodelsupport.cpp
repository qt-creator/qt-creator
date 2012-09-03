/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qtuicodemodelsupport.h"
#include "qt4buildconfiguration.h"

#include "qt4project.h"
#include "qt4projectmanager.h"
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

using namespace Qt4ProjectManager;
using namespace Internal;

Qt4UiCodeModelSupport::Qt4UiCodeModelSupport(CPlusPlus::CppModelManagerInterface *modelmanager,
                                             Qt4Project *project,
                                             const QString &source,
                                             const QString &uiHeaderFile)
    : CppTools::UiCodeModelSupport(modelmanager, source, uiHeaderFile),
      m_project(project)
{ }

Qt4UiCodeModelSupport::~Qt4UiCodeModelSupport()
{ }

QString Qt4UiCodeModelSupport::uicCommand() const
{
    QtSupport::BaseQtVersion *version;
    if (m_project->needsConfiguration()) {
        version = QtSupport::QtKitInformation::qtVersion(ProjectExplorer::KitManager::instance()->defaultKit());
    } else {
        ProjectExplorer::Target *target = m_project->activeTarget();
        version = QtSupport::QtKitInformation::qtVersion(target->kit());
    }
    return version ? version->uicCommand() : QString();
}

QStringList Qt4UiCodeModelSupport::environment() const
{
    if (m_project->needsConfiguration()) {
        return Utils::Environment::systemEnvironment().toStringList();
    } else {
        ProjectExplorer::Target *target = m_project->activeTarget();
        if (!target)
            return QStringList();
        ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration();
        return bc ? bc->environment().toStringList() : QStringList();
    }
}
