// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidqmltoolingsupport.h"

#include "androidconstants.h"
#include "androidrunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace Tasking;

namespace Android::Internal {

class AndroidQmlToolingSupportFactory final : public RunWorkerFactory
{
public:
    AndroidQmlToolingSupportFactory()
    {
        setId("AndroidQmlToolingSupportFactory");
        setRecipeProducer([](RunControl *runControl) {
            return Group {
                parallel,
                androidRecipe(runControl),
                runControl->createRecipe(ProjectExplorer::Constants::QML_PROFILER_RUNNER)
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
    }
};

void setupAndroidQmlToolingSupport()
{
    static AndroidQmlToolingSupportFactory theAndroidQmlToolingSupportFactory;
}

} // Android::Internal
