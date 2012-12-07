/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
#include <qtsupport/qtkitinformation.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/fileutils.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <utils/filesystemwatcher.h>

#include <QTextStream>
#include <QDeclarativeComponent>
#include <QDebug>

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

void QmlProject::addedTarget(ProjectExplorer::Target *target)
{
    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    foreach (ProjectExplorer::RunConfiguration *rc, target->runConfigurations())
        addedRunConfiguration(rc);
}

void QmlProject::addedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    // The enabled state of qml runconfigurations can only be decided after
    // they have been added to a project
    QmlProjectRunConfiguration *qmlrc = qobject_cast<QmlProjectRunConfiguration *>(rc);
    if (qmlrc)
        qmlrc->updateEnabled();
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
    pinfo.importPaths = customImportPaths();
    QtSupport::BaseQtVersion *version = 0;
    if (activeTarget()) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(activeTarget()->kit());
        version = QtSupport::QtKitInformation::qtVersion(activeTarget()->kit());
        QtSupport::QmlDumpTool::pathAndEnvironment(this, version, tc, false, &pinfo.qmlDumpPath, &pinfo.qmlDumpEnvironment);
    }
    if (version) {
        pinfo.tryQmlDump = true;
        pinfo.qtImportsPath = version->qmakeProperty("QT_INSTALL_IMPORTS");
        pinfo.qtQmlPath = version->qmakeProperty("QT_INSTALL_QML");
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

QStringList QmlProject::customImportPaths() const
{
    QStringList importPaths;
    if (m_projectItem)
        importPaths = m_projectItem.data()->importPaths();

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

bool QmlProject::supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const
{
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
    if (deviceType != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (errorMessage)
            *errorMessage = tr("Device type is not desktop.");
        return false;
    }

    // TODO: Limit supported versions?
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version && errorMessage)
        *errorMessage = tr("No Qt version set in kit.");
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

    ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::instance()->defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    refresh(Everything);

    // addedTarget calls updateEnabled on the runconfigurations
    // which needs to happen after refresh
    foreach (ProjectExplorer::Target *t, targets())
        addedTarget(t);

    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(addedTarget(ProjectExplorer::Target*)));

    return true;
}

} // namespace QmlProjectManager

