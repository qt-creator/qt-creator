/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsbuildsystem.h"

#include "makefileparserthread.h"
#include "makestep.h"

#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtcppkitinfo.h>

#include <utils/filesystemwatcher.h>

using namespace ProjectExplorer;

namespace AutotoolsProjectManager {
namespace Internal {

AutotoolsBuildSystem::AutotoolsBuildSystem(Target *target)
    : BuildSystem(target)
    , m_cppCodeModelUpdater(new CppTools::CppProjectUpdater)
{
    connect(target, &Target::activeBuildConfigurationChanged, this, [this]() { requestParse(); });

    connect(target->project(), &Project::projectFileIsDirty, this, [this]() { requestParse(); });
}

AutotoolsBuildSystem::~AutotoolsBuildSystem()
{
    delete m_cppCodeModelUpdater;

    if (m_makefileParserThread) {
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = nullptr;
    }
}

void AutotoolsBuildSystem::triggerParsing()
{
    if (m_makefileParserThread) {
        // The thread is still busy parsing a previous configuration.
        // Wait until the thread has been finished and delete it.
        // TODO: Discuss whether blocking is acceptable.
        disconnect(m_makefileParserThread,
                   &QThread::finished,
                   this,
                   &AutotoolsBuildSystem::makefileParsingFinished);
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = nullptr;
    }

    // Parse the makefile asynchronously in a thread
    m_makefileParserThread = new MakefileParserThread(this);

    connect(m_makefileParserThread,
            &MakefileParserThread::finished,
            this,
            &AutotoolsBuildSystem::makefileParsingFinished);
    m_makefileParserThread->start();
}

void AutotoolsBuildSystem::makefileParsingFinished()
{
    // The finished() signal is from a previous makefile-parser-thread
    // and can be skipped. This can happen, if the thread has emitted the
    // finished() signal during the execution of AutotoolsBuildSystem::loadProjectTree().
    // In this case the signal is in the message queue already and deleting
    // the thread of course does not remove the signal again.
    if (sender() != m_makefileParserThread)
        return;

    if (m_makefileParserThread->isCanceled()) {
        // The parsing has been cancelled by the user. Don't show any
        // project data at all.
        m_makefileParserThread->deleteLater();
        m_makefileParserThread = nullptr;
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
    foreach (const QString& file, files)
        m_files.append(dir.absoluteFilePath(file));

    // Watch for changes of Makefile.am files. If a Makefile.am file
    // has been changed, the project tree must be reparsed.
    const QStringList makefiles = m_makefileParserThread->makefiles();
    foreach (const QString &makefile, makefiles) {
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
    for (const QString &f : m_files) {
        const Utils::FilePath path = Utils::FilePath::fromString(f);
        newRoot->addNestedNode(std::make_unique<FileNode>(path,
                                                          FileNode::fileTypeForFileName(path)));
    }
    setRootProjectNode(std::move(newRoot));
    project()->setExtraProjectFiles(filesToWatch);

    updateCppCodeModel();

    m_makefileParserThread->deleteLater();
    m_makefileParserThread = nullptr;

    emitBuildSystemUpdated();
}

static QStringList filterIncludes(const QString &absSrc, const QString &absBuild,
                                  const QStringList &in)
{
    QStringList result;
    foreach (const QString i, in) {
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
    rpp.setFlagsForC({kitInfo.cToolChain, cflags});
    rpp.setFlagsForCxx({kitInfo.cxxToolChain, cxxflags});

    const QString absSrc = project()->projectDirectory().toString();
    BuildConfiguration *bc = target()->activeBuildConfiguration();

    const QString absBuild = bc ? bc->buildDirectory().toString() : QString();

    rpp.setIncludePaths(filterIncludes(absSrc, absBuild, m_makefileParserThread->includePaths()));
    rpp.setMacros(m_makefileParserThread->macros());
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), {rpp}});
}

} // namespace Internal
} // namespace AutotoolsProjectManager
