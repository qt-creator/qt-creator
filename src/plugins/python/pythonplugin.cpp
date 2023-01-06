// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonplugin.h"

#include "pysidebuildconfiguration.h"
#include "pythoneditor.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythonrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/taskhub.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;

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

bool PythonPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new PythonPluginPrivate;

    ProjectManager::registerProjectType<PythonProject>(PythonMimeType);

    return true;
}

void PythonPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    QString imageFile = Utils::creatorTheme()->imageFile(Utils::Theme::IconOverlayPro,
                                                         ::Constants::FILEOVERLAY_PY);
    Utils::FileIconProvider::registerIconOverlayForSuffix(imageFile, "py");

    TaskHub::addCategory(PythonErrorTaskCategory, "Python", true);
}

} // Python::Internal
