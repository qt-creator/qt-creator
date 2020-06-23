/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "makecommandbuilder.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/project.h>

#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>

using namespace ProjectExplorer;

namespace IncrediBuild {
namespace Internal {

bool MakeCommandBuilder::canMigrate(BuildStepList *buildStepList)
{
    // "Make"
    QString makeClassName("QmakeProjectManager::QmakeMakeStep");
    for (int i = buildStepList->count() - 1; i >= 0; --i) {
        QString className = QString::fromUtf8(buildStepList->at(i)->metaObject()->className());
        if (className.compare(makeClassName) == 0) {
            buildStepList->at(i)->setEnabled(false);
            buildStepList->at(i)->projectConfiguration()->project()->saveSettings();
            return true;
        }
    }
    return false;
}

QString MakeCommandBuilder::defaultCommand()
{
    if (!m_defaultMake.isEmpty())
        return m_defaultMake;

    BuildConfiguration *buildConfig = buildStep()->buildConfiguration();
    if (buildConfig) {
        Target *target = buildStep()->target();
        if (target) {
            ToolChain *toolChain = ToolChainKitAspect::toolChain(target->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            if (toolChain)
                m_defaultMake = toolChain->makeCommand(buildConfig->environment()).toString();
        }
    }

    return m_defaultMake;
}


QString MakeCommandBuilder::setMultiProcessArg(QString args)
{
    QString cmd = command();
    QFileInfo fileInfo(cmd);

    // jom -j 200
    if (fileInfo.baseName().compare("jom", Qt::CaseSensitivity::CaseInsensitive) == 0) {
        QRegularExpression regExp("\\s*\\-j\\s+\\d+");
        args.remove(regExp);
        args.append(" -j 200");
     }
    // make -j200
    else if ((fileInfo.baseName().compare("make", Qt::CaseSensitivity::CaseInsensitive) == 0)
          || (fileInfo.baseName().compare("gmake", Qt::CaseSensitivity::CaseInsensitive) == 0)) {
        QRegularExpression regExp("\\s*\\-j\\d+");
        args.remove(regExp);
        args.append(" -j200");
    }

    return args;
}

} // namespace Internal
} // namespace IncrediBuild
