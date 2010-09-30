/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmldumptool.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <qtconcurrent/runextensions.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QHash>

namespace {
using namespace Qt4ProjectManager;

class QmlDumpBuildTask;

typedef QHash<int, QmlDumpBuildTask *> QmlDumpByVersion;
Q_GLOBAL_STATIC(QmlDumpByVersion, runningQmlDumpBuilds);

// A task suitable to be run by QtConcurrent to build qmldump.
class QmlDumpBuildTask : public QObject {
    Q_DISABLE_COPY(QmlDumpBuildTask)
    Q_OBJECT
public:
    explicit QmlDumpBuildTask(QtVersion *version)
        : m_version(*version)
    {
        runningQmlDumpBuilds()->insert(m_version.uniqueId(), this);
    }

    void run(QFutureInterface<void> &future)
    {
        future.setProgressRange(0, 5);
        future.setProgressValue(1);
        const QString output = m_version.buildDebuggingHelperLibrary(future, true);

        const QString qtInstallData = m_version.versionInfo().value("QT_INSTALL_DATA");
        QString path = QmlDumpTool::toolByInstallData(qtInstallData);
        if (path.isEmpty()) {
            qWarning() << "Could not build QML plugin dumping helper for " << m_version.displayName()
                       << "\nOutput:\n" << output;
        }

        // proceed in gui thread
        metaObject()->invokeMethod(this, "finish", Qt::QueuedConnection, Q_ARG(QString, path));
    }

    void updateProjectWhenDone(ProjectExplorer::Project *project)
    {
        m_projectsToUpdate.insert(project);
    }

public slots:
    void finish(QString qmldumpPath)
    {
        deleteLater();
        runningQmlDumpBuilds()->remove(m_version.uniqueId());

        if (qmldumpPath.isEmpty())
            return;

        // update qmldump path for all the project
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        if (!modelManager)
            return;

        foreach (ProjectExplorer::Project *project, m_projectsToUpdate) {
            QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager->projectInfo(project);
            if (projectInfo.qmlDumpPath.isEmpty()) {
                projectInfo.qmlDumpPath = qmldumpPath;
                modelManager->updateProjectInfo(projectInfo);
            }
        }
    }

private:
    QSet<ProjectExplorer::Project *> m_projectsToUpdate;
    QtVersion m_version;
};
} // end of anonymous namespace


namespace Qt4ProjectManager {

static inline QStringList validBinaryFilenames()
{
    return QStringList()
            << QLatin1String("debug/qmldump.exe")
            << QLatin1String("qmldump.exe")
            << QLatin1String("qmldump")
            << QLatin1String("qmldump.app/Contents/MacOS/qmldump");
}

bool QmlDumpTool::canBuild(QtVersion *qtVersion)
{
    const QString installHeaders = qtVersion->versionInfo().value("QT_INSTALL_HEADERS");
    const QString header = installHeaders + QLatin1String("/QtDeclarative/private/qdeclarativemetatype_p.h");
    return checkMinimumQtVersion(qtVersion->qtVersionString(), 4, 7, 1) || QFile::exists(header);
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

    // else, find any desktop Qt version that has qmldump, or - if there isn't any -
    // one that could build it
    QtVersion *desktopQt = 0;
    QtVersionManager *qtVersions = QtVersionManager::instance();
    foreach (QtVersion *version, qtVersions->validVersions()) {
        if (version->supportsTargetId(Constants::DESKTOP_TARGET_ID)) {
            const QString qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
            const QString path = QmlDumpTool::toolByInstallData(qtInstallData);
            if (!path.isEmpty())
                return version;

            if (!desktopQt && QmlDumpTool::canBuild(version)) {
                desktopQt = version;
            }
        }
    }

    return desktopQt;
}

QString QmlDumpTool::toolForProject(ProjectExplorer::Project *project)
{
    QtVersion *version = qtVersionForProject(project);
    if (version) {
        QString qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
        QString toolPath = toolByInstallData(qtInstallData);
        return toolPath;
    }

    return QString();
}

QString QmlDumpTool::toolByInstallData(const QString &qtInstallData)
{
    if (!Core::ICore::instance())
        return QString();

    const QString mainFilename = Core::ICore::instance()->resourcePath()
            + QLatin1String("/qml/qmldump/main.cpp");
    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames();

    return byInstallDataHelper(mainFilename, directories, binFilenames);
}

QStringList QmlDumpTool::locationsByInstallData(const QString &qtInstallData)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames();
    foreach(const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

QString QmlDumpTool::build(const QString &directory, const QString &makeCommand,
                     const QString &qmakeCommand, const QString &mkspec,
                     const Utils::Environment &env, const QString &targetMode)
{
    return buildHelper(QCoreApplication::tr("qmldump"), QLatin1String("qmldump.pro"),
                       directory, makeCommand, qmakeCommand, mkspec, env, targetMode);
}

QString QmlDumpTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlDumpTool::installDirectories(qtInstallData);

    QStringList files;
    files << QLatin1String("main.cpp") << QLatin1String("qmldump.pro")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");

    QString sourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmldump/");

    // Try to find a writeable directory.
    foreach(const QString &directory, directories)
        if (copyFiles(sourcePath, files, directory, errorMessage)) {
            errorMessage->clear();
            return directory;
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

QString QmlDumpTool::qmlDumpPath(ProjectExplorer::Project *project)
{
    QString path;

    path = Qt4ProjectManager::QmlDumpTool::toolForProject(project);

    QtVersion *version = qtVersionForProject(project);
    if (version && path.isEmpty()) {
        if (runningQmlDumpBuilds()->contains(version->uniqueId())) {
            runningQmlDumpBuilds()->value(version->uniqueId())->updateProjectWhenDone(project);
        } else {
            QmlDumpBuildTask *buildTask = new QmlDumpBuildTask(version);
            buildTask->updateProjectWhenDone(project);
            QFuture<void> task = QtConcurrent::run(&QmlDumpBuildTask::run, buildTask);
            const QString taskName = QmlDumpBuildTask::tr("Building helper");
            Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                                QLatin1String("Qt4ProjectManager::BuildHelpers"));
        }
        return path;
    }

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

    return path;
}

} // namespace Qt4ProjectManager

#include "qmldumptool.moc"
