/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlproject.h"
#include "qmlprojectfile.h"
#include "qmlprojectmanagerconstants.h"
#include "fileformat/qmlprojectitem.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include "qmlprojectnodes.h"
#include "qmlprojectmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/documentmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtprofileinformation.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/fileutils.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/profilemanager.h>
#include <projectexplorer/target.h>
#include <utils/filesystemwatcher.h>

#include <QTextStream>
#include <QDeclarativeComponent>
#include <QtDebug>

namespace QmlProjectManager {

QmlProject::QmlProject(Internal::Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_modelManager(QmlJS::ModelManagerInterface::instance())
{
    setProjectContext(Core::Context(QmlProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguage(Core::Context(QmlProjectManager::Constants::LANG_QML));

    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = new Internal::QmlProjectFile(this, fileName);
    m_rootNode = new Internal::QmlProjectNode(this, m_file);

    Core::DocumentManager::addDocument(m_file, true);

    m_manager->registerProject(this);
}

QmlProject::~QmlProject()
{
    m_manager->unregisterProject(this);

    Core::DocumentManager::removeDocument(m_file);

    delete m_projectItem.data();
    delete m_rootNode;
}

QDir QmlProject::projectDir() const
{
    return QFileInfo(document()->fileName()).dir();
}

QString QmlProject::filesFileName() const
{ return m_fileName; }

void QmlProject::parseProject(RefreshOptions options)
{
    Core::MessageManager *messageManager = Core::ICore::messageManager();
    if (options & Files) {
        if (options & ProjectFile)
            delete m_projectItem.data();
        if (!m_projectItem) {
            Utils::FileReader reader;
            if (reader.fetch(m_fileName)) {
                QDeclarativeComponent *component = new QDeclarativeComponent(&m_engine, this);
                component->setData(reader.data(), QUrl::fromLocalFile(m_fileName));
                if (component->isReady()
                    && qobject_cast<QmlProjectItem*>(component->create())) {
                    m_projectItem = qobject_cast<QmlProjectItem*>(component->create());
                    connect(m_projectItem.data(), SIGNAL(qmlFilesChanged(QSet<QString>,QSet<QString>)),
                            this, SLOT(refreshFiles(QSet<QString>,QSet<QString>)));
                } else {
                    messageManager->printToOutputPane(tr("Error while loading project file %1.").arg(m_fileName));
                    messageManager->printToOutputPane(component->errorString(), true);
                }
            } else {
                messageManager->printToOutputPane(tr("QML project: %1").arg(reader.errorString()), true);
            }
        }
        if (m_projectItem) {
            m_projectItem.data()->setSourceDirectory(projectDir().path());
            m_modelManager->updateSourceFiles(m_projectItem.data()->files(), true);

            QString mainFilePath = m_projectItem.data()->mainFile();
            if (!mainFilePath.isEmpty()) {
                mainFilePath = projectDir().absoluteFilePath(mainFilePath);
                if (!QFileInfo(mainFilePath).isReadable()) {
                    messageManager->printToOutputPane(
                                tr("Warning while loading project file %1.").arg(m_fileName));
                    messageManager->printToOutputPane(
                                tr("File '%1' does not exist or is not readable.").arg(mainFilePath), true);
                }
            }
        }
        m_rootNode->refresh();
    }

    if (options & Configuration) {
        // update configuration
    }

    if (options & Files)
        emit fileListChanged();
}

void QmlProject::refresh(RefreshOptions options)
{
    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    QmlJS::ModelManagerInterface::ProjectInfo pinfo(this);
    pinfo.sourceFiles = files();
    pinfo.importPaths = importPaths();
    QtSupport::BaseQtVersion *version = 0;
    if (activeTarget()) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(activeTarget()->profile());
        version = QtSupport::QtProfileInformation::qtVersion(activeTarget()->profile());
        QtSupport::QmlDumpTool::pathAndEnvironment(this, version, tc, false, &pinfo.qmlDumpPath, &pinfo.qmlDumpEnvironment);
    }
    if (version) {
        pinfo.tryQmlDump = true;
        pinfo.qtImportsPath = version->qmakeProperty("QT_INSTALL_IMPORTS");
        pinfo.qtVersionString = version->qtVersionString();
    }
    m_modelManager->updateProjectInfo(pinfo);
}

QStringList QmlProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(QFileInfo(m_fileName).dir());
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(projectDir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList QmlProject::files() const
{
    QStringList files;
    if (m_projectItem) {
        files = m_projectItem.data()->files();
    } else {
        files = m_files;
    }
    return files;
}

QString QmlProject::mainFile() const
{
    if (m_projectItem)
        return m_projectItem.data()->mainFile();
    return QString();
}

bool QmlProject::validProjectFile() const
{
    return !m_projectItem.isNull();
}

QStringList QmlProject::importPaths() const
{
    QStringList importPaths;
    if (m_projectItem)
        importPaths = m_projectItem.data()->importPaths();

    // add the default import path for the target Qt version
    if (activeTarget()) {
        const QmlProjectRunConfiguration *runConfig =
                qobject_cast<QmlProjectRunConfiguration*>(activeTarget()->activeRunConfiguration());
        if (runConfig) {
            const QtSupport::BaseQtVersion *qtVersion = runConfig->qtVersion();
            if (qtVersion && qtVersion->isValid()) {
                const QString qtVersionImportPath = qtVersion->qmakeProperty("QT_INSTALL_IMPORTS");
                if (!qtVersionImportPath.isEmpty())
                    importPaths += qtVersionImportPath;
            }
        }
    }

    return importPaths;
}

bool QmlProject::addFiles(const QStringList &filePaths)
{
    QStringList toAdd;
    foreach (const QString &filePath, filePaths) {
        if (!m_projectItem.data()->matchesFile(filePath))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

void QmlProject::refreshProjectFile()
{
    refresh(QmlProject::ProjectFile | Files);
}

void QmlProject::refreshFiles(const QSet<QString> &/*added*/, const QSet<QString> &removed)
{
    refresh(Files);
    if (!removed.isEmpty())
        m_modelManager->removeFiles(removed.toList());
}

QString QmlProject::displayName() const
{
    return m_projectName;
}

Core::Id QmlProject::id() const
{
    return Core::Id("QmlProjectManager.QmlProject");
}

Core::IDocument *QmlProject::document() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *QmlProject::projectManager() const
{
    return m_manager;
}

bool QmlProject::supportsProfile(ProjectExplorer::Profile *p) const
{
    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(p);
    if (deviceType != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return false;

    // TODO: Limit supported versions?
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);
    return version;
}

QList<ProjectExplorer::BuildConfigWidget*> QmlProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildConfigWidget*>();
}

ProjectExplorer::ProjectNode *QmlProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList QmlProject::files(FilesMode) const
{
    return files();
}

bool QmlProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    if (!activeTarget())
        addTarget(createTarget(ProjectExplorer::ProfileManager::instance()->defaultProfile()));

    refresh(Everything);
    // FIXME workaround to guarantee that run/debug actions are enabled if a valid file exists
    if (activeTarget()) {
        QmlProjectRunConfiguration *runConfig = qobject_cast<QmlProjectRunConfiguration*>(activeTarget()->activeRunConfiguration());
        if (runConfig)
            runConfig->changeCurrentFile(0);
    }

    return true;
}

} // namespace QmlProjectManager

