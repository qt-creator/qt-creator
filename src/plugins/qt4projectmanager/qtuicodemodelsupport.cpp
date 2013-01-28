/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
