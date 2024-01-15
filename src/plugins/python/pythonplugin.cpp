// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonplugin.h"

#include "pythonbuildconfiguration.h"
#include "pythonconstants.h"
#include "pythoneditor.h"
#include "pythonkitaspect.h"
#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonwizardpage.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/taskhub.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static QObject *m_instance = nullptr;

QObject *pluginInstance()
{
    return m_instance;
}

class PythonPluginPrivate
{
public:
    PythonOutputFormatterFactory outputFormatterFactory;
    PythonRunConfigurationFactory runConfigFactory;
    PySideBuildStepFactory buildStepFactory;
    PythonBuildConfigurationFactory buildConfigFactory;
    SimpleTargetRunnerFactory runWorkerFactory{{runConfigFactory.runConfigurationId()}};
    PythonSettings settings;
    PythonWizardPageFactory pythonWizardPageFactory;
};

class PythonPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Python.json")

public:
    PythonPlugin()
    {
        m_instance = this;
    }

    ~PythonPlugin() final
    {
        m_instance = nullptr;
        delete d;
    }

private:
    void initialize() final
    {
        d = new PythonPluginPrivate;

        setupPythonEditorFactory(this);

        KitManager::setIrrelevantAspects(KitManager::irrelevantAspects()
                                         + QSet<Id>{PythonKitAspect::id()});

        ProjectManager::registerProjectType<PythonProject>(Constants::C_PY_PROJECT_MIME_TYPE);
        ProjectManager::registerProjectType<PythonProject>(Constants::C_PY_PROJECT_MIME_TYPE_LEGACY);
    }

    void extensionsInitialized() final
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

    PythonPluginPrivate *d = nullptr;
};

} // Python::Internal

#include "pythonplugin.moc"
