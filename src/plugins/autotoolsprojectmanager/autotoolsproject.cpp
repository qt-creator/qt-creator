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
#include "autotoolsmanager.h"
#include "autotoolsprojectnode.h"
#include "autotoolsprojectfile.h"
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
#include <cpptools/projectpartbuilder.h>
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

AutotoolsProject::AutotoolsProject(AutotoolsManager *manager, const QString &fileName) :
    m_fileWatcher(new Utils::FileSystemWatcher(this))
{
    setId(Constants::AUTOTOOLS_PROJECT_ID);
    setProjectManager(manager);
    setDocument(new AutotoolsProjectFile(fileName));
    setRootProjectNode(new AutotoolsProjectNode(projectDirectory()));
    setProjectContext(Core::Context(Constants::PROJECT_CONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    const QFileInfo fileInfo = projectFilePath().toFileInfo();
    m_projectName = fileInfo.absoluteDir().dirName();
    rootProjectNode()->setDisplayName(m_projectName);
}

AutotoolsProject::~AutotoolsProject()
{
    setRootProjectNode(0);

    if (m_makefileParserThread != 0) {
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = 0;
    }
}

QString AutotoolsProject::displayName() const
{
    return m_projectName;
}

QString AutotoolsProject::defaultBuildDirectory(const QString &projectPath)
{
    return QFileInfo(projectPath).absolutePath();
}

QStringList AutotoolsProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    return m_files;
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
    if (m_makefileParserThread != 0) {
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
        m_makefileParserThread = 0;
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
    QStringList files = m_makefileParserThread->sources();
    foreach (const QString& file, files)
        m_files.append(dir.absoluteFilePath(file));

    // Watch for changes of Makefile.am files. If a Makefile.am file
    // has been changed, the project tree must be reparsed.
    const QStringList makefiles = m_makefileParserThread->makefiles();
    foreach (const QString &makefile, makefiles) {
        files.append(makefile);

        const QString watchedFile = dir.absoluteFilePath(makefile);
        m_fileWatcher->addFile(watchedFile, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(watchedFile);
    }

    // Add configure.ac file to project and watch for changes.
    const QLatin1String configureAc(QLatin1String("configure.ac"));
    const QFile configureAcFile(fileInfo.absolutePath() + QLatin1Char('/') + configureAc);
    if (configureAcFile.exists()) {
        files.append(configureAc);
        const QString configureAcFilePath = dir.absoluteFilePath(configureAc);
        m_fileWatcher->addFile(configureAcFilePath, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(configureAcFilePath);
    }

    QList<FileNode *> fileNodes = Utils::transform(files, [dir](const QString &f) {
        const Utils::FileName path = Utils::FileName::fromString(dir.absoluteFilePath(f));
        return new FileNode(path,
                            (f == QLatin1String("Makefile.am") ||
                             f == QLatin1String("configure.ac")) ? FileType::Project : FileType::Resource,
                            false);
    });
    rootProjectNode()->buildTree(fileNodes);

    updateCppCodeModel();

    m_makefileParserThread->deleteLater();
    m_makefileParserThread = 0;

    emit parsingFinished();
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
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    m_codeModelFuture.cancel();
    CppTools::ProjectInfo pInfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pInfo);

    ppBuilder.setProjectFile(projectFilePath().toString());

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit())) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    ppBuilder.setQtVersion(activeQtVersion);
    const QStringList cflags = m_makefileParserThread->cflags();
    QStringList cxxflags = m_makefileParserThread->cxxflags();
    if (cxxflags.isEmpty())
        cxxflags = cflags;
    ppBuilder.setCFlags(cflags);
    ppBuilder.setCxxFlags(cxxflags);

    const QString absSrc = projectDirectory().toString();
    const Target *target = activeTarget();
    const QString absBuild = (target && target->activeBuildConfiguration())
            ? target->activeBuildConfiguration()->buildDirectory().toString() : QString();

    ppBuilder.setIncludePaths(filterIncludes(absSrc, absBuild, m_makefileParserThread->includePaths()));
    ppBuilder.setDefines(m_makefileParserThread->defines());

    const QList<Core::Id> languages = ppBuilder.createProjectPartsForFiles(m_files);
    foreach (Core::Id language, languages)
        setProjectLanguage(language, true);

    m_codeModelFuture = modelManager->updateProjectInfo(pInfo);
}
