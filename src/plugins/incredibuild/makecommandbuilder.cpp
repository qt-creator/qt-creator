// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makecommandbuilder.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/project.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h> // Compile-time only

#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild {
namespace Internal {

QList<Utils::Id> MakeCommandBuilder::migratableSteps() const
{
    return {QmakeProjectManager::Constants::MAKESTEP_BS_ID};
}

Utils::FilePath MakeCommandBuilder::defaultCommand() const
{
    BuildConfiguration *buildConfig = buildStep()->buildConfiguration();
    if (buildConfig) {
        if (Target *target = buildStep()->target()) {
            if (ToolChain *toolChain = ToolChainKitAspect::cxxToolChain(target->kit()))
                return toolChain->makeCommand(buildConfig->environment());
        }
    }

    return {};
}

QString MakeCommandBuilder::setMultiProcessArg(QString args)
{
    const FilePath cmd = command();

    // jom -j 200
    if (cmd.baseName().compare("jom", Qt::CaseSensitivity::CaseInsensitive) == 0) {
        QRegularExpression regExp("\\s*\\-j\\s+\\d+");
        args.remove(regExp);
        args.append(" -j 200");
     }
    // make -j200
    else if ((cmd.baseName().compare("make", Qt::CaseSensitivity::CaseInsensitive) == 0)
          || (cmd.baseName().compare("gmake", Qt::CaseSensitivity::CaseInsensitive) == 0)) {
        QRegularExpression regExp("\\s*\\-j\\d+");
        args.remove(regExp);
        args.append(" -j200");
    }

    return args;
}

} // namespace Internal
} // namespace IncrediBuild
