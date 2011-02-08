/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmldumptool.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"
#include "debugginghelperbuildtask.h"
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <qtconcurrent/runextensions.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/qtcassert.h>
#include <QtGui/QDesktopServices>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QHash>

namespace {

using namespace Qt4ProjectManager;
using Qt4ProjectManager::Internal::DebuggingHelperBuildTask;


class QmlDumpBuildTask;

typedef QHash<int, QmlDumpBuildTask *> QmlDumpByVersion;
Q_GLOBAL_STATIC(QmlDumpByVersion, qmlDumpBuilds)

// A task suitable to be run by QtConcurrent to build qmldump.
class QmlDumpBuildTask : public QObject {
    Q_DISABLE_COPY(QmlDumpBuildTask)
    Q_OBJECT
public:
    explicit QmlDumpBuildTask(QtVersion *version)
        : m_buildTask(new DebuggingHelperBuildTask(version, DebuggingHelperBuildTask::QmlDump))
        , m_failed(false)
    {
        qmlDumpBuilds()->insert(m_version.uniqueId(), this);

        connect(m_buildTask, SIGNAL(finished(int,DebuggingHelperBuildTask::Tools,QString)),
                this, SLOT(finish(int,DebuggingHelperBuildTask::Tools,QString)),
                Qt::QueuedConnection);
    }

    void run(QFutureInterface<void> &future)
    {
        m_buildTask->run(future);
    }

    void updateProjectWhenDone(ProjectExplorer::Project *project, bool preferDebug)
    {
        m_projectsToUpdate.insert(qMakePair(project, preferDebug));
    }

    bool hasFailed() const
    {
        return m_failed;
    }

private slots:
    void finish(int qtId, DebuggingHelperBuildTask::Tools tools, const QString &output)
    {
        QtVersion *version = QtVersionManager::instance()->version(qtId);

        QTC_ASSERT(tools == DebuggingHelperBuildTask::QmlDump, return);
        QString errorMessage;
        if (!version) {
            m_failed = true;
            errorMessage = QString::fromLatin1("Qt version became invalid");
        } else {
            version->invalidateCache();

            if (!version->hasQmlDump()) {
                m_failed = true;
                errorMessage = QString::fromLatin1("Could not build QML plugin dumping helper for %1\n"
                                                   "Output:\n%2").
                        arg(version->displayName(), output);
            }
        }

        if (m_failed) {
            qWarning("%s", qPrintable(errorMessage));
        }

        // update qmldump path for all the project
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        if (!modelManager)
            return;

        typedef QPair<ProjectExplorer::Project *, bool> ProjectAndDebug;
        foreach (ProjectAndDebug projectAndDebug, m_projectsToUpdate) {
            QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager->projectInfo(projectAndDebug.first);
            projectInfo.qmlDumpPath = version->qmlDumpTool(projectAndDebug.second);
            if (projectInfo.qmlDumpPath.isEmpty())
                projectInfo.qmlDumpPath = version->qmlDumpTool(!projectAndDebug.second);
            projectInfo.qmlDumpEnvironment = version->qmlToolsEnvironment();
            modelManager->updateProjectInfo(projectInfo);
        }

        // clean up
        qmlDumpBuilds()->remove(qtId);
        deleteLater();
    }

private:
    QSet<QPair<ProjectExplorer::Project *, bool> > m_projectsToUpdate;
    Internal::DebuggingHelperBuildTask *m_buildTask; // deletes itself after run()
    QtVersion m_version;
    bool m_failed;
};
} // end of anonymous namespace


namespace Qt4ProjectManager {

static inline QStringList validBinaryFilenames(bool debugBuild)
{
    QStringList list = QStringList()
            << QLatin1String("qmldump.exe")
            << QLatin1String("qmldump")
            << QLatin1String("qmldump.app/Contents/MacOS/qmldump");
    if (debugBuild)
        list.prepend(QLatin1String("debug/qmldump.exe"));
    else
        list.prepend(QLatin1String("release/qmldump.exe"));
    return list;
}

bool QmlDumpTool::canBuild(const QtVersion *qtVersion)
{
    const QString installHeaders = qtVersion->versionInfo().value("QT_INSTALL_HEADERS");
    const QString header = installHeaders + QLatin1String("/QtDeclarative/private/qdeclarativemetatype_p.h");
    return (qtVersion->supportsTargetId(Constants::DESKTOP_TARGET_ID)
            || (qtVersion->supportsTargetId(Constants::QT_SIMULATOR_TARGET_ID)
                && checkMinimumQtVersion(qtVersion->qtVersionString(), 4, 7, 1)))
           && QFile::exists(header);
}

static QtVersion *qtVersionForProject(ProjectExplorer::Project *project)
{
    if (project && project->id() == Qt4ProjectManager::Constants::QT4PROJECT_ID) {
        Qt4Project *qt4Project = static_cast<Qt4Project*>(project);
        if (qt4Project && qt4Project->activeTarget()
                && qt4Project->activeTarget()->activeBuildConfiguration()) {
            QtVersion *version = qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion();
            if (version->isValid())
                return version;
        }
        return 0;
    }

    if (project && project->id() == QLatin1String("QmlProjectManager.QmlProject")) {
        // We cannot access the QmlProject interfaces here, therefore use the metatype system
        if (!project->activeTarget() || !project->activeTarget()->activeRunConfiguration())
            return 0;
        QVariant variant = project->activeTarget()->activeRunConfiguration()->property("qtVersionId");
        QTC_ASSERT(variant.isValid() && variant.canConvert(QVariant::Int), return 0);
        QtVersion *version = QtVersionManager::instance()->version(variant.toInt());
        if (version && version->isValid())
            return version;
        return 0;
    }

    // else, find any desktop or simulator Qt version that has qmldump, or
    // - if there isn't any - one that could build it
    QtVersion *canBuildQmlDump = 0;
    QtVersionManager *qtVersions = QtVersionManager::instance();
    foreach (QtVersion *version, qtVersions->validVersions()) {
        if (version->supportsTargetId(Constants::DESKTOP_TARGET_ID)
                || version->supportsTargetId(Constants::QT_SIMULATOR_TARGET_ID)) {
            if (version->hasQmlDump())
                return version;

            if (!canBuildQmlDump && QmlDumpTool::canBuild(version)) {
                canBuildQmlDump = version;
            }
        }
    }

    return canBuildQmlDump;
}

QString QmlDumpTool::toolForProject(ProjectExplorer::Project *project, bool debugDump)
{
    QtVersion *version = qtVersionForProject(project);
    if (version) {
        QString qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
        QString toolPath = toolByInstallData(qtInstallData, debugDump);
        return toolPath;
    }

    return QString();
}

QString QmlDumpTool::toolByInstallData(const QString &qtInstallData, bool debugDump)
{
    if (!Core::ICore::instance())
        return QString();

    const QString mainFilename = Core::ICore::instance()->resourcePath()
            + QLatin1String("/qml/qmldump/main.cpp");
    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames(debugDump);

    return byInstallDataHelper(mainFilename, directories, binFilenames);
}

QStringList QmlDumpTool::locationsByInstallData(const QString &qtInstallData, bool debugDump)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames(debugDump);
    foreach(const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

bool QmlDumpTool::build(const QString &directory, const QString &makeCommand,
                        const QString &qmakeCommand, const QString &mkspec,
                        const Utils::Environment &env, const QString &targetMode,
                        QString *output,  QString *errorMessage)
{
    return buildHelper(QCoreApplication::translate("Qt4ProjectManager::QmlDumpTool", "qmldump"), QLatin1String("qmldump.pro"),
                       directory, makeCommand, qmakeCommand, mkspec, env, targetMode,
                       output, errorMessage);
}

QString QmlDumpTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlDumpTool::installDirectories(qtInstallData);

    QStringList files;
    files << QLatin1String("main.cpp") << QLatin1String("qmldump.pro")
          << QLatin1String("qmlstreamwriter.cpp") << QLatin1String("qmlstreamwriter.h")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT")
          << QLatin1String("Info.plist");

    QString sourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmldump/");

    // Try to find a writeable directory.
    foreach(const QString &directory, directories) {
        if (copyFiles(sourcePath, files, directory, errorMessage)) {
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlDumpTool",
                                                "qmldump could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlDumpTool::installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmldump/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmldump/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-qmldump/") + QString::number(hash)) + slash;
    return directories;
}

void QmlDumpTool::pathAndEnvironment(ProjectExplorer::Project *project, bool preferDebug,
                                     QString *dumperPath, Utils::Environment *env)
{
    QString path;

    QtVersion *version = qtVersionForProject(project);
    if (version && !version->hasQmlDump() && QmlDumpTool::canBuild(version)) {
        QmlDumpBuildTask *qmlDumpBuildTask = qmlDumpBuilds()->value(version->uniqueId());
        if (qmlDumpBuildTask) {
            if (!qmlDumpBuildTask->hasFailed())
                qmlDumpBuildTask->updateProjectWhenDone(project, preferDebug);
        } else {
            QmlDumpBuildTask *buildTask = new QmlDumpBuildTask(version);
            buildTask->updateProjectWhenDone(project, preferDebug);
            QFuture<void> task = QtConcurrent::run(&QmlDumpBuildTask::run, buildTask);
            const QString taskName = QmlDumpBuildTask::tr("Building helper");
            Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                                QLatin1String("Qt4ProjectManager::BuildHelpers"));
        }
        return;
    }

    path = Qt4ProjectManager::QmlDumpTool::toolForProject(project, preferDebug);
    if (path.isEmpty())
        path = Qt4ProjectManager::QmlDumpTool::toolForProject(project, !preferDebug);

    if (!path.isEmpty()) {
        QFileInfo qmldumpFileInfo(path);
        if (!qmldumpFileInfo.exists()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: qmldump executable does not exist at" << path;
            path.clear();
        } else if (!qmldumpFileInfo.isFile()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: " << path << " is not a file";
            path.clear();
        }
    }

    if (!path.isEmpty() && version && dumperPath && env) {
        *dumperPath = path;
        *env = version->qmlToolsEnvironment();
    }
}

} // namespace Qt4ProjectManager

#include "qmldumptool.moc"
