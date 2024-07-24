// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonbuildconfiguration.h"

#include "pipsupport.h"
#include "pyside.h"
#include "pysideuicextracompiler.h"
#include "pythonconstants.h"
#include "pythoneditor.h"
#include "pythonkitaspect.h"
#include "pythonlanguageclient.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/editormanager/documentmodel.h>

#include <languageclient/languageclientmanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

PySideBuildStep::PySideBuildStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    m_pysideProject.setSettingsKey("Python.PySideProjectTool");
    m_pysideProject.setLabelText(Tr::tr("PySide project tool:"));
    m_pysideProject.setToolTip(Tr::tr("Enter location of PySide project tool."));
    m_pysideProject.setExpectedKind(PathChooser::Command);
    m_pysideProject.setHistoryCompleter("Python.PySideProjectTool.History");
    m_pysideProject.setReadOnly(true);

    m_pysideUic.setSettingsKey("Python.PySideUic");
    m_pysideUic.setLabelText(Tr::tr("PySide uic tool:"));
    m_pysideUic.setToolTip(Tr::tr("Enter location of PySide uic tool."));
    m_pysideUic.setExpectedKind(PathChooser::Command);
    m_pysideUic.setHistoryCompleter("Python.PySideUic.History");
    m_pysideUic.setReadOnly(true);

    setCommandLineProvider([this] { return CommandLine(m_pysideProject(), {"build"}); });
    setWorkingDirectoryProvider([this] {
        return m_pysideProject().withNewMappedPath(project()->projectDirectory()); // FIXME: new path needed?
    });
    setEnvironmentModifier([this](Environment &env) {
        env.prependOrSetPath(m_pysideProject().parentDir());
    });

    connect(target(), &Target::buildSystemUpdated, this, &PySideBuildStep::updateExtraCompilers);
    connect(&m_pysideUic, &BaseAspect::changed, this, &PySideBuildStep::updateExtraCompilers);
}

PySideBuildStep::~PySideBuildStep()
{
    qDeleteAll(m_extraCompilers);
}

void PySideBuildStep::checkForPySide(const FilePath &python)
{
    PySideTools tools;
    if (python.isEmpty() || !python.isExecutableFile()) {
        m_pysideProject.setValue(FilePath());
        m_pysideUic.setValue(FilePath());
        return;
    }
    const FilePath dir = python.parentDir();
    tools.pySideProjectPath = dir.pathAppended("pyside6-project").withExecutableSuffix();
    tools.pySideUicPath = dir.pathAppended("pyside6-uic").withExecutableSuffix();

    if (tools.pySideProjectPath.isExecutableFile() && tools.pySideUicPath.isExecutableFile()) {
        m_pysideProject.setValue(tools.pySideProjectPath.toUserOutput());
        m_pysideUic.setValue(tools.pySideUicPath.toUserOutput());
    } else {
        checkForPySide(python, "PySide6-Essentials");
    }
}

void PySideBuildStep::checkForPySide(const FilePath &python, const QString &pySidePackageName)
{
    const PipPackage package(pySidePackageName);
    QObject::disconnect(m_watcherConnection);
    m_watcher.reset(new QFutureWatcher<PipPackageInfo>());
    m_watcherConnection = QObject::connect(m_watcher.get(), &QFutureWatcherBase::finished, this,
                                           [this, python, pySidePackageName] {
        handlePySidePackageInfo(m_watcher->result(), python, pySidePackageName);
    });
    const auto future = Pip::instance(python)->info(package);
    m_watcher->setFuture(future);
    Utils::futureSynchronizer()->addFuture(future);
}

void PySideBuildStep::handlePySidePackageInfo(const PipPackageInfo &pySideInfo,
                                              const FilePath &python,
                                              const QString &requestedPackageName)
{
    const auto findPythonTools = [](const FilePaths &files,
                                    const FilePath &location,
                                    const FilePath &python) -> PySideTools {
        PySideTools result;
        const QString pySide6ProjectName
            = OsSpecificAspects::withExecutableSuffix(python.osType(), "pyside6-project");
        const QString pySide6UicName
            = OsSpecificAspects::withExecutableSuffix(python.osType(), "pyside6-uic");
        for (const FilePath &file : files) {
            if (file.fileName() == pySide6ProjectName) {
                result.pySideProjectPath = python.withNewMappedPath(location.resolvePath(file));
                result.pySideProjectPath = result.pySideProjectPath.cleanPath();
                if (!result.pySideUicPath.isEmpty())
                    return result;
            } else if (file.fileName() == pySide6UicName) {
                result.pySideUicPath = python.withNewMappedPath(location.resolvePath(file));
                result.pySideUicPath = result.pySideUicPath.cleanPath();
                if (!result.pySideProjectPath.isEmpty())
                    return result;
            }
        }
        return {};
    };

    PySideTools tools = findPythonTools(pySideInfo.files, pySideInfo.location, python);
    if (!tools.pySideProjectPath.isExecutableFile() && requestedPackageName != "PySide6") {
        checkForPySide(python, "PySide6");
        return;
    }

    m_pysideProject.setValue(tools.pySideProjectPath.toUserOutput());
    m_pysideUic.setValue(tools.pySideUicPath.toUserOutput());
}

Tasking::GroupItem PySideBuildStep::runRecipe()
{
    using namespace Tasking;

    const auto onSetup = [this] {
        if (!processParameters()->effectiveCommand().isExecutableFile())
            return SetupResult::StopWithSuccess;
        return SetupResult::Continue;
    };

    return Group { onGroupSetup(onSetup), defaultProcessTask() };
}

void PySideBuildStep::updateExtraCompilers()
{
    QList<PySideUicExtraCompiler *> oldCompilers = m_extraCompilers;
    m_extraCompilers.clear();

    if (m_pysideUic().isExecutableFile()) {
        auto uiMatcher = [](const Node *node) {
            if (const FileNode *fileNode = node->asFileNode())
                return fileNode->fileType() == FileType::Form;
            return false;
        };
        const FilePaths uiFiles = project()->files(uiMatcher);
        for (const FilePath &uiFile : uiFiles) {
            FilePath generated = uiFile.parentDir();
            generated = generated.pathAppended("/ui_" + uiFile.baseName() + ".py");
            int index = Utils::indexOf(oldCompilers, [&](PySideUicExtraCompiler *oldCompiler) {
                return oldCompiler->pySideUicPath() == m_pysideUic()
                       && oldCompiler->project() == project() && oldCompiler->source() == uiFile
                       && oldCompiler->targets() == FilePaths{generated};
            });
            if (index < 0) {
                m_extraCompilers << new PySideUicExtraCompiler(m_pysideUic(),
                                                               project(),
                                                               uiFile,
                                                               {generated},
                                                               this);
            } else {
                m_extraCompilers << oldCompilers.takeAt(index);
            }
        }
    }
    for (LanguageClient::Client *client : LanguageClient::LanguageClientManager::clients()) {
        if (auto pylsClient = qobject_cast<PyLSClient *>(client))
            pylsClient->updateExtraCompilers(project(), m_extraCompilers);
    }
    qDeleteAll(oldCompilers);
}

QList<PySideUicExtraCompiler *> PySideBuildStep::extraCompilers() const
{
    return m_extraCompilers;
}

Id PySideBuildStep::id()
{
    return Id("Python.PysideBuildStep");
}

class PythonBuildSettingsWidget : public NamedWidget
{
public:
    PythonBuildSettingsWidget(PythonBuildConfiguration *bc)
        : NamedWidget(Tr::tr("Python"))
    {
        using namespace Layouting;
        m_configureDetailsWidget = new DetailsWidget;
        m_configureDetailsWidget->setSummaryText(bc->python().toUserOutput());

        if (const std::optional<FilePath> venv = bc->venv()) {
            auto details = new QWidget();
            Form{Tr::tr("Effective venv:"), venv->toUserOutput(), br}.attachTo(details);
            m_configureDetailsWidget->setWidget(details);
        } else {
            m_configureDetailsWidget->setState(DetailsWidget::OnlySummary);
        }

        Column{
            m_configureDetailsWidget,
            noMargin
        }.attachTo(this);
    }
private:
    DetailsWidget *m_configureDetailsWidget;
};

class PySideBuildStepFactory final : public BuildStepFactory
{
public:
    PySideBuildStepFactory()
    {
        registerStep<PySideBuildStep>(PySideBuildStep::id());
        setSupportedProjectType(PythonProjectId);
        setDisplayName(Tr::tr("Run PySide6 project tool"));
        setFlags(BuildStep::UniqueStep);
    }
};

void setupPySideBuildStep()
{
    static PySideBuildStepFactory thePySideBuildStepFactory;
}

// PythonBuildConfiguration

PythonBuildConfiguration::PythonBuildConfiguration(Target *target, const Id &id)
    : BuildConfiguration(target, id)
    , m_buildSystem(std::make_unique<PythonBuildSystem>(this))
{
    setInitializer([this](const BuildInfo &info) { initialize(info); });

    updateCacheAndEmitEnvironmentChanged();

    connect(&PySideInstaller::instance(),
            &PySideInstaller::pySideInstalled,
            this,
            &PythonBuildConfiguration::handlePythonUpdated);

    auto update = [this] {
        if (isActive()) {
            m_buildSystem->emitBuildSystemUpdated();
            updateDocuments();
        }
    };
    connect(target, &Target::activeBuildConfigurationChanged, this, update);
    connect(project(), &Project::activeTargetChanged, this, update);
    connect(ProjectExplorerPlugin::instance(),
            &ProjectExplorerPlugin::fileListChanged,
            this,
            update);
    connect(PythonSettings::instance(),
            &PythonSettings::virtualEnvironmentCreated,
            this,
            &PythonBuildConfiguration::handlePythonUpdated);
}

NamedWidget *PythonBuildConfiguration::createConfigWidget()
{
    return new PythonBuildSettingsWidget(this);
}

static QString venvTypeName()
{
    static QString name = Tr::tr("New Virtual Environment");
    return name;
}

void PythonBuildConfiguration::initialize(const BuildInfo &info)
{
    buildSteps()->appendStep(PySideBuildStep::id());
    if (info.typeName == venvTypeName()) {
        m_venv = info.buildDirectory;
        const FilePath venvInterpreterPath = info.buildDirectory.resolvePath(
            HostOsInfo::isWindowsHost() ? FilePath::fromUserInput("Scripts/python.exe")
                                        : FilePath::fromUserInput("bin/python"));

        updatePython(venvInterpreterPath);

        if (info.extraInfo.toMap().value("createVenv", false).toBool()
            && !info.buildDirectory.exists()) {
            if (std::optional<Interpreter> python = PythonKitAspect::python(target()->kit()))
                PythonSettings::createVirtualEnvironment(python->command, info.buildDirectory);
        }
    } else {
        updateInterpreter(PythonKitAspect::python(target()->kit()));
    }

    updateCacheAndEmitEnvironmentChanged();
}

void PythonBuildConfiguration::updateInterpreter(const std::optional<Interpreter> &python)
{
    updatePython(python ? python->command : FilePath());
}

void PythonBuildConfiguration::updatePython(const FilePath &python)
{
    m_python = python;
    if (auto buildStep = buildSteps()->firstOfType<PySideBuildStep>())
        buildStep->checkForPySide(python);
    updateDocuments();
    m_buildSystem->requestParse();
}

void PythonBuildConfiguration::updateDocuments()
{
    if (isActive()) {
        const FilePaths files = project()->files(Project::AllFiles);
        for (const FilePath &file : files) {
            if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(file)) {
                if (auto pyDoc = qobject_cast<PythonDocument *>(doc))
                    pyDoc->updatePython(m_python);
                else if (doc->mimeType() == Utils::Constants::QML_MIMETYPE)
                    PySideInstaller::instance().checkPySideInstallation(m_python, doc);
            }
        }
    }
}

void PythonBuildConfiguration::handlePythonUpdated(const FilePath &python)
{
    if (!m_python.isEmpty() && python == m_python)
        updatePython(python); // retrigger pyside check
}

static const char pythonKey[] = "python";
static const char venvKey[] = "venv";

void PythonBuildConfiguration::fromMap(const Store &map)
{
    BuildConfiguration::fromMap(map);
    if (map.contains(venvKey))
        m_venv = FilePath::fromSettings(map[venvKey]);
    updatePython(FilePath::fromSettings(map[pythonKey]));
}

void PythonBuildConfiguration::toMap(Store &map) const
{
    BuildConfiguration::toMap(map);
    map[pythonKey] = m_python.toSettings();
    if (m_venv)
        map[venvKey] = m_venv->toSettings();
}

BuildSystem *PythonBuildConfiguration::buildSystem() const
{
    return m_buildSystem.get();
}

FilePath PythonBuildConfiguration::python() const
{
    return m_python;
}

std::optional<FilePath> PythonBuildConfiguration::venv() const
{
    return m_venv;
}

class PythonBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    PythonBuildConfigurationFactory()
    {
        registerBuildConfiguration<PythonBuildConfiguration>("Python.PySideBuildConfiguration");
        setSupportedProjectType(PythonProjectId);
        setSupportedProjectMimeTypeName(Constants::C_PY_PROJECT_MIME_TYPE);
        setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
            if (std::optional<Interpreter> python = PythonKitAspect::python(k)) {
                BuildInfo base;
                base.buildDirectory = projectPath.parentDir();
                base.displayName = python->name;
                base.typeName = Tr::tr("Global Python");
                base.showBuildDirConfigWidget = false;

                if (isVenvPython(python->command) || !venvIsUsable(python->command))
                    return QList<BuildInfo>{base};

                base.enabledByDefault = false;

                BuildInfo venv;
                const FilePath venvBase = projectPath.parentDir() / ".qtcreator"
                                          / FileUtils::fileSystemFriendlyName(python->name + "venv");
                venv.buildDirectory = venvBase;
                int i = 2;
                while (venv.buildDirectory.exists())
                    venv.buildDirectory = venvBase.stringAppended('_' + QString::number(i++));
                venv.displayName = python->name + Tr::tr(" Virtual Environment");
                venv.typeName = venvTypeName();
                venv.extraInfo = QVariantMap{{"createVenv", forSetup}};
                return QList<BuildInfo>{base, venv};
            }
            return QList<BuildInfo>{};
        });
    }
};

void setupPythonBuildConfiguration()
{
    static PythonBuildConfigurationFactory thePythonBuildConfigurationFactory;
}

} // Python::Internal
