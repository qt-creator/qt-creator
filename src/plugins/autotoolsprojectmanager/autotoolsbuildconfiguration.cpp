// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotoolsbuildconfiguration.h"

#include "autotoolsprojectconstants.h"
#include "autotoolsprojectmanagertr.h"
#include "makefileparser.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtcppkitinfo.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/async.h>
#include <utils/mimeconstants.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// AutotoolsBuildSystem
class AutotoolsBuildSystem final : public BuildSystem
{
public:
    explicit AutotoolsBuildSystem(BuildConfiguration *bc);

private:
    void triggerParsing() final;

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been finished. Adds all sources and files into the project tree and
     * takes care listen to file changes for Makefile.am and configure.ac
     * files.
     */
    void makefileParsingFinished(const MakefileParserOutputData &outputData);

    /// Return value for AutotoolsProject::files()
    FilePaths m_files;

    /// Responsible for parsing the makefiles asynchronously in a thread
    Tasking::TaskTreeRunner m_parserRunner;

    std::unique_ptr<ProjectUpdater> m_cppCodeModelUpdater;
};

AutotoolsBuildSystem::AutotoolsBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
{
    const auto reparseIfActive = [this] {
        if (target()->activeBuildConfiguration() == buildConfiguration())
            requestDelayedParse();
    };
    connect(project(), &Project::projectFileIsDirty, this, reparseIfActive);
    connect(target(), &Target::activeBuildConfigurationChanged, this, reparseIfActive);
}

static void parseMakefileImpl(QPromise<MakefileParserOutputData> &promise, const QString &makefile)
{
    const auto result = parseMakefile(makefile, QFuture<void>(promise.future()));
    if (result)
        promise.addResult(*result);
    else
        promise.future().cancel();
}

void AutotoolsBuildSystem::triggerParsing()
{
    const Storage<std::optional<ParseGuard>> storage;

    const auto onSetup = [this, storage](Async<MakefileParserOutputData> &async) {
        *storage = guardParsingRun();
        async.setConcurrentCallData(parseMakefileImpl, projectFilePath().path());
    };
    const auto onDone = [this, storage](const Async<MakefileParserOutputData> &async) {
        (*storage)->markAsSuccess();
        makefileParsingFinished(async.result());
    };

    const Group recipe {
        storage,
        AsyncTask<MakefileParserOutputData>(onSetup, onDone, CallDoneIf::Success)
    };
    m_parserRunner.start(recipe);
}

static QStringList filterIncludes(const QString &absSrc, const QString &absBuild,
                                  const QStringList &in)
{
    QStringList result;
    for (const QString &i : in) {
        QString out = i;
        out.replace(QLatin1String("$(top_srcdir)"), absSrc);
        out.replace(QLatin1String("$(abs_top_srcdir)"), absSrc);

        out.replace(QLatin1String("$(top_builddir)"), absBuild);
        out.replace(QLatin1String("$(abs_top_builddir)"), absBuild);

        result << out;
    }
    return result;
}

void AutotoolsBuildSystem::makefileParsingFinished(const MakefileParserOutputData &outputData)
{
    m_files.clear();

    QSet<FilePath> filesToWatch;

    // Apply sources to m_files, which are returned at AutotoolsBuildSystem::files()
    const FilePath dir = projectFilePath().parentDir();
    const QStringList files = outputData.m_sources;
    for (const QString &file : files)
        m_files.append(dir.pathAppended(file));

    // Watch for changes of Makefile.am files. If a Makefile.am file
    // has been changed, the project tree must be reparsed.
    const QStringList makefiles = outputData.m_makefiles;
    for (const QString &makefile : makefiles) {
        const FilePath absMakefile = dir.pathAppended(makefile);
        m_files.append(absMakefile);
        filesToWatch.insert(absMakefile);
    }

    // Add configure.ac file to project and watch for changes.
    const FilePath configureAcFile(dir.pathAppended("configure.ac"));
    if (configureAcFile.exists()) {
        m_files.append(configureAcFile);
        filesToWatch.insert(configureAcFile);
    }

    auto newRoot = std::make_unique<ProjectNode>(project()->projectDirectory());
    for (const FilePath &file : std::as_const(m_files)) {
        newRoot->addNestedNode(std::make_unique<FileNode>(file,
                                                          FileNode::fileTypeForFileName(file)));
    }
    setRootProjectNode(std::move(newRoot));
    project()->setExtraProjectFiles(filesToWatch);

    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return );

    RawProjectPart rpp;
    rpp.setDisplayName(project()->displayName());
    rpp.setProjectFileLocation(projectFilePath());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    const QStringList cflags = outputData.m_cflags;
    QStringList cxxflags = outputData.m_cxxflags;
    if (cxxflags.isEmpty())
        cxxflags = cflags;

    const FilePath includeFileBaseDir = projectDirectory();
    rpp.setFlagsForC({kitInfo.cToolchain, cflags, includeFileBaseDir});
    rpp.setFlagsForCxx({kitInfo.cxxToolchain, cxxflags, includeFileBaseDir});

    const QString absSrc = project()->projectDirectory().path();
    BuildConfiguration *bc = target()->activeBuildConfiguration();

    const QString absBuild = bc ? bc->buildDirectory().path() : QString();

    rpp.setIncludePaths(filterIncludes(absSrc, absBuild, outputData.m_includePaths));
    rpp.setMacros(outputData.m_macros);
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), {rpp}});

    emitBuildSystemUpdated();
}

// AutotoolsBuildConfiguration
class AutotoolsBuildConfiguration final : public BuildConfiguration
{
public:
    AutotoolsBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        // /<foobar> is used so the un-changed check in setBuildDirectory() works correctly.
        // The leading / is to avoid the relative the path expansion in BuildConfiguration::buildDirectory.
        setBuildDirectory("/<foobar>");
        setBuildDirectoryHistoryCompleter("AutoTools.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Autotools Manager"));

        // ### Build Steps Build ###
        const FilePath autogenFile = project()->projectDirectory() / "autogen.sh";
        if (autogenFile.exists())
            appendInitialBuildStep(Constants::AUTOGEN_STEP_ID); // autogen.sh
        else
            appendInitialBuildStep(Constants::AUTORECONF_STEP_ID); // autoreconf

        appendInitialBuildStep(Constants::CONFIGURE_STEP_ID); // ./configure.
        appendInitialBuildStep(Constants::MAKE_STEP_ID); // make

        // ### Build Steps Clean ###
        appendInitialCleanStep(Constants::MAKE_STEP_ID); // make clean
    }
};

class AutotoolsBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    AutotoolsBuildConfigurationFactory()
    {
        registerBuildConfiguration<AutotoolsBuildConfiguration>
                ("AutotoolsProjectManager.AutotoolsBuildConfiguration");

        setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
        setSupportedProjectMimeTypeName(Utils::Constants::MAKEFILE_MIMETYPE);

        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            BuildInfo info;
            info.typeName = ::ProjectExplorer::Tr::tr("Build");
            info.buildDirectory = forSetup ? projectPath.parentDir() : projectPath;
            if (forSetup) {
                //: The name of the build configuration created by default for a autotools project.
                info.displayName = ::ProjectExplorer::Tr::tr("Default");
            }
            return QList<BuildInfo>{info};
        });
    }
};

/**
 * @brief Implementation of the ProjectExplorer::Project interface.
 *
 * Loads the autotools project and embeds it into the QtCreator project tree.
 * The class AutotoolsProject is the core of the autotools project plugin.
 * It is responsible to parse the Makefile.am files and do trigger project
 * updates if a Makefile.am file or a configure.ac file has been changed.
 */
class AutotoolsProject : public Project
{
public:
    explicit AutotoolsProject(const Utils::FilePath &fileName)
        : Project(Utils::Constants::MAKEFILE_MIMETYPE, fileName)
    {
        setType(Constants::AUTOTOOLS_PROJECT_ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());
        setHasMakeInstallEquivalent(true);
        setBuildSystemCreator<AutotoolsBuildSystem>("autotools");
    }
};

void setupAutotoolsProject()
{
    static AutotoolsBuildConfigurationFactory theAutotoolsBuildConfigurationFactory;
    ProjectManager::registerProjectType<AutotoolsProject>(Utils::Constants::MAKEFILE_MIMETYPE);
}

} // AutotoolsProjectManager::Internal
