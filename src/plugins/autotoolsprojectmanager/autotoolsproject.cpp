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

#include "autotoolsproject.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsprojectnode.h"
#include "autotoolsopenprojectwizard.h"
#include "makestep.h"
#include "makefileparserthread.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/headerpath.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cppprojectupdater.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/filesystemwatcher.h>

#include <QFileInfo>
#include <QTimer>
#include <QPointer>
#include <QApplication>
#include <QCursor>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

AutotoolsProject::AutotoolsProject(const Utils::FileName &fileName) :
    Project(Constants::MAKEFILE_MIMETYPE, fileName),
    m_fileWatcher(new Utils::FileSystemWatcher(this)),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater(this))
{
    setId(Constants::AUTOTOOLS_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
}

AutotoolsProject::~AutotoolsProject()
{
    delete m_cppCodeModelUpdater;

    setRootProjectNode(nullptr);

    if (m_makefileParserThread) {
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = nullptr;
    }
}

QString AutotoolsProject::defaultBuildDirectory(const QString &projectPath)
{
    return QFileInfo(projectPath).absolutePath();
}

// This function, is called at the very beginning, to
// restore the settings if there are some stored.
Project::RestoreResult AutotoolsProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    connect(m_fileWatcher, &Utils::FileSystemWatcher::fileChanged,
            this, &AutotoolsProject::onFileChanged);

    // Load the project tree structure.
    loadProjectTree();

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    return RestoreResult::Ok;
}

void AutotoolsProject::loadProjectTree()
{
    emitParsingStarted();
    if (m_makefileParserThread) {
        // The thread is still busy parsing a previus configuration.
        // Wait until the thread has been finished and delete it.
        // TODO: Discuss whether blocking is acceptable.
        disconnect(m_makefileParserThread, &QThread::finished,
                   this, &AutotoolsProject::makefileParsingFinished);
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = 0;
    }

    // Parse the makefile asynchronously in a thread
    m_makefileParserThread = new MakefileParserThread(projectFilePath().toString());

    connect(m_makefileParserThread, &MakefileParserThread::started,
            this, &AutotoolsProject::makefileParsingStarted);

    connect(m_makefileParserThread, &MakefileParserThread::finished,
            this, &AutotoolsProject::makefileParsingFinished);
    m_makefileParserThread->start();
}

void AutotoolsProject::makefileParsingStarted()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void AutotoolsProject::makefileParsingFinished()
{
    // The finished() signal is from a previous makefile-parser-thread
    // and can be skipped. This can happen, if the thread has emitted the
    // finished() signal during the execution of AutotoolsProject::loadProjectTree().
    // In this case the signal is in the message queue already and deleting
    // the thread of course does not remove the signal again.
    if (sender() != m_makefileParserThread)
        return;

    QApplication::restoreOverrideCursor();

    if (m_makefileParserThread->isCanceled()) {
        // The parsing has been cancelled by the user. Don't show any
        // project data at all.
        m_makefileParserThread->deleteLater();
        m_makefileParserThread = nullptr;
        return;
    }

    if (m_makefileParserThread->hasError())
        qWarning("Parsing of makefile contained errors.");

    // Remove file watches for the current project state.
    // The file watches will be added again after the parsing.
    foreach (const QString& watchedFile, m_watchedFiles)
        m_fileWatcher->removeFile(watchedFile);

    m_watchedFiles.clear();

    // Apply sources to m_files, which are returned at AutotoolsProject::files()
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

        m_fileWatcher->addFile(absMakefile, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(absMakefile);
    }

    // Add configure.ac file to project and watch for changes.
    const QLatin1String configureAc(QLatin1String("configure.ac"));
    const QFile configureAcFile(fileInfo.absolutePath() + QLatin1Char('/') + configureAc);
    if (configureAcFile.exists()) {
        const QString absConfigureAc = dir.absoluteFilePath(configureAc);
        m_files.append(absConfigureAc);

        m_fileWatcher->addFile(absConfigureAc, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(absConfigureAc);
    }

    auto newRoot = std::make_unique<AutotoolsProjectNode>(projectDirectory());
    for (const QString &f : m_files) {
        const Utils::FileName path = Utils::FileName::fromString(f);
        newRoot->addNestedNode(std::make_unique<FileNode>(path, FileNode::fileTypeForFileName(path),
                                                          false));
    }
    setRootProjectNode(std::move(newRoot));

    updateCppCodeModel();

    m_makefileParserThread->deleteLater();
    m_makefileParserThread = nullptr;

    emitParsingFinished(true);
}

void AutotoolsProject::onFileChanged(const QString &file)
{
    Q_UNUSED(file);
    loadProjectTree();
}

QStringList AutotoolsProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
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

void AutotoolsProject::updateCppCodeModel()
{
    const Kit *k = nullptr;
    if (Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::defaultKit();
    QTC_ASSERT(k, return);

    ToolChain *cToolChain
            = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);
    ToolChain *cxxToolChain
            = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    m_cppCodeModelUpdater->cancel();

    CppTools::RawProjectPart rpp;
    rpp.setDisplayName(displayName());
    rpp.setProjectFileLocation(projectFilePath().toString());

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k)) {
        if (qtVersion->qtVersion() <= QtSupport::QtVersionNumber(4,8,6))
            activeQtVersion = CppTools::ProjectPart::Qt4_8_6AndOlder;
        else if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4Latest;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    rpp.setQtVersion(activeQtVersion);
    const QStringList cflags = m_makefileParserThread->cflags();
    QStringList cxxflags = m_makefileParserThread->cxxflags();
    if (cxxflags.isEmpty())
        cxxflags = cflags;
    rpp.setFlagsForC({cToolChain, cflags});
    rpp.setFlagsForCxx({cxxToolChain, cxxflags});

    const QString absSrc = projectDirectory().toString();
    const Target *target = activeTarget();
    const QString absBuild = (target && target->activeBuildConfiguration())
            ? target->activeBuildConfiguration()->buildDirectory().toString() : QString();

    rpp.setIncludePaths(filterIncludes(absSrc, absBuild, m_makefileParserThread->includePaths()));
    rpp.setMacros(m_makefileParserThread->macros());
    rpp.setFiles(m_files);

    m_cppCodeModelUpdater->update({this, cToolChain, cxxToolChain, k, {rpp}});
}
