// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotoolsbuildsystem.h"

#include "makefileparserthread.h"

#include <cppeditor/cppprojectupdater.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtcppkitinfo.h>

#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

AutotoolsBuildSystem::AutotoolsBuildSystem(Target *target)
    : BuildSystem(target)
    , m_cppCodeModelUpdater(new CppEditor::CppProjectUpdater)
{
    connect(target, &Target::activeBuildConfigurationChanged, this, [this] { requestParse(); });
    connect(target->project(), &Project::projectFileIsDirty, this, [this] { requestParse(); });
}

AutotoolsBuildSystem::~AutotoolsBuildSystem()
{
    delete m_cppCodeModelUpdater;

    if (m_makefileParserThread)
        m_makefileParserThread->wait();
}

void AutotoolsBuildSystem::triggerParsing()
{
    // The thread is still busy parsing a previous configuration.
    // Wait until the thread has been finished and delete it.
    // TODO: Discuss whether blocking is acceptable.
    if (m_makefileParserThread)
        m_makefileParserThread->wait();

    // Parse the makefile asynchronously in a thread
    m_makefileParserThread.reset(new MakefileParserThread(this));

    connect(m_makefileParserThread.get(), &MakefileParserThread::done,
            this, &AutotoolsBuildSystem::makefileParsingFinished);
    m_makefileParserThread->start();
}

void AutotoolsBuildSystem::makefileParsingFinished()
{
    // The parsing has been cancelled by the user. Don't show any project data at all.
    if (m_makefileParserThread->isCanceled()) {
        m_makefileParserThread.release()->deleteLater();
        return;
    }

    if (m_makefileParserThread->hasError())
        qWarning("Parsing of makefile contained errors.");

    m_files.clear();

    QSet<Utils::FilePath> filesToWatch;

    // Apply sources to m_files, which are returned at AutotoolsBuildSystem::files()
    const QFileInfo fileInfo = projectFilePath().toFileInfo();
    const QDir dir = fileInfo.absoluteDir();
    const QStringList files = m_makefileParserThread->sources();
    for (const QString& file : files)
        m_files.append(dir.absoluteFilePath(file));

    // Watch for changes of Makefile.am files. If a Makefile.am file
    // has been changed, the project tree must be reparsed.
    const QStringList makefiles = m_makefileParserThread->makefiles();
    for (const QString &makefile : makefiles) {
        const QString absMakefile = dir.absoluteFilePath(makefile);

        m_files.append(absMakefile);

        filesToWatch.insert(Utils::FilePath::fromString(absMakefile));
    }

    // Add configure.ac file to project and watch for changes.
    const QLatin1String configureAc(QLatin1String("configure.ac"));
    const QFile configureAcFile(fileInfo.absolutePath() + QLatin1Char('/') + configureAc);
    if (configureAcFile.exists()) {
        const QString absConfigureAc = dir.absoluteFilePath(configureAc);
        m_files.append(absConfigureAc);

        filesToWatch.insert(Utils::FilePath::fromString(absConfigureAc));
    }

    auto newRoot = std::make_unique<ProjectNode>(project()->projectDirectory());
    for (const QString &f : std::as_const(m_files)) {
        const Utils::FilePath path = Utils::FilePath::fromString(f);
        newRoot->addNestedNode(std::make_unique<FileNode>(path,
                                                          FileNode::fileTypeForFileName(path)));
    }
    setRootProjectNode(std::move(newRoot));
    project()->setExtraProjectFiles(filesToWatch);

    updateCppCodeModel();

    m_makefileParserThread.release()->deleteLater();

    emitBuildSystemUpdated();
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

void AutotoolsBuildSystem::updateCppCodeModel()
{
    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return );

    RawProjectPart rpp;
    rpp.setDisplayName(project()->displayName());
    rpp.setProjectFileLocation(projectFilePath().toString());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    const QStringList cflags = m_makefileParserThread->cflags();
    QStringList cxxflags = m_makefileParserThread->cxxflags();
    if (cxxflags.isEmpty())
        cxxflags = cflags;

    const FilePath includeFileBaseDir = projectDirectory();
    rpp.setFlagsForC({kitInfo.cToolChain, cflags, includeFileBaseDir});
    rpp.setFlagsForCxx({kitInfo.cxxToolChain, cxxflags, includeFileBaseDir});

    const QString absSrc = project()->projectDirectory().toString();
    BuildConfiguration *bc = target()->activeBuildConfiguration();

    const QString absBuild = bc ? bc->buildDirectory().toString() : QString();

    rpp.setIncludePaths(filterIncludes(absSrc, absBuild, m_makefileParserThread->includePaths()));
    rpp.setMacros(m_makefileParserThread->macros());
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), {rpp}});
}

} // AutotoolsProjectManager::Internal
