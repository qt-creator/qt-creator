// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonplugin.h"

#include "pysidebuildconfiguration.h"
#include "pythoneditor.h"
#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonwizardpage.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/taskhub.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static PythonPlugin *m_instance = nullptr;

class PythonPluginPrivate
{
public:
    PythonEditorFactory editorFactory;
    PythonOutputFormatterFactory outputFormatterFactory;
    PythonRunConfigurationFactory runConfigFactory;
    PySideBuildStepFactory buildStepFactory;
    PySideBuildConfigurationFactory buildConfigFactory;
    SimpleTargetRunnerFactory runWorkerFactory{{runConfigFactory.runConfigurationId()}};
    PythonSettings settings;
    PythonWizardPageFactory pythonWizardPageFactory;
};

PythonPlugin::PythonPlugin()
{
    m_instance = this;
}

PythonPlugin::~PythonPlugin()
{
    m_instance = nullptr;
    delete d;
}

PythonPlugin *PythonPlugin::instance()
{
    return m_instance;
}

void PythonPlugin::initialize()
{
    d = new PythonPluginPrivate;

    ProjectManager::registerProjectType<PythonProject>(PythonMimeType);
    ProjectManager::registerProjectType<PythonProject>(PythonMimeTypeLegacy);
}

void PythonPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QString imageFile = Utils::creatorTheme()->imageFile(Theme::IconOverlayPro,
                                                               ::Constants::FILEOVERLAY_PY);
    FileIconProvider::registerIconOverlayForSuffix(imageFile, "py");

    TaskHub::addCategory({PythonErrorTaskCategory,
                          "Python",
                          Tr::tr("Issues parsed from Python runtime output."),
                          true});
}

} // Python::Internal
