/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlproject.h"
#include "qmlprojectfile.h"
#include "qmlprojectmanagerconstants.h"
#include "fileformat/qmlprojectitem.h"

//<<<<<<< HEAD:src/plugins/qmlprojectmanager/qmlproject.cpp
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/filewatcher.h>
#include <qmljseditor/qmljsmodelmanagerinterface.h>

#include <QTextStream>
#include <QmlComponent>
#include <QtDebug>

namespace QmlProjectManager {

QmlProject::QmlProject(Internal::Manager *manager, const QString &fileName)
/*=======
#include "qmlinspector/qmlinspector.h"
#include "qmlinspector/qmlinspectorconstants.h"

#include <debugger/debuggeruiswitcher.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <qmljseditor/qmlmodelmanagerinterface.h>

#include <utils/synchronousprocess.h>

#include <QtCore/QtDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>

#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>

#include <QtDeclarative/QmlComponent>

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const QML_RC_ID("QmlProjectManager.QmlRunConfiguration");
const char * const QML_RC_DISPLAY_NAME(QT_TRANSLATE_NOOP("QmlProjectManager::Internal::QmlRunConfiguration", "QML Viewer"));

const char * const QML_VIEWER_KEY("QmlProjectManager.QmlRunConfiguration.QmlViewer");
const char * const QML_VIEWER_ARGUMENTS_KEY("QmlProjectManager.QmlRunConfiguration.QmlViewerArguments");
const char * const QML_MAINSCRIPT_KEY("QmlProjectManager.QmlRunConfiguration.MainScript");
const char * const QML_DEBUG_SERVER_ADDRESS_KEY("QmlProjectManager.QmlRunConfiguration.DebugServerAddress");
const char * const QML_DEBUG_SERVER_PORT_KEY("QmlProjectManager.QmlRunConfiguration.DebugServerPort");

const int DEFAULT_DEBUG_SERVER_PORT(3768);
} // namespace

////////////////////////////////////////////////////////////////////////////////////
// QmlProject
////////////////////////////////////////////////////////////////////////////////////

QmlProject::QmlProject(Manager *manager, const QString &fileName)
>>>>>>> debugintegration:src/plugins/qmlprojectmanager/qmlproject.cpp
*/
    : m_manager(manager),
      m_fileName(fileName),
      m_modelManager(ExtensionSystem::PluginManager::instance()->getObject<QmlJSEditor::ModelManagerInterface>()),
      m_fileWatcher(new ProjectExplorer::FileWatcher(this)),
      m_targetFactory(new Internal::QmlProjectTargetFactory(this))
{
    setSupportedTargetIds(QSet<QString>() << QLatin1String(Constants::QML_VIEWER_TARGET_ID));
    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = new Internal::QmlProjectFile(this, fileName);
    m_rootNode = new Internal::QmlProjectNode(this, m_file);

    m_fileWatcher->addFile(fileName),
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(refreshProjectFile()));

    m_manager->registerProject(this);
}

QmlProject::~QmlProject()
{
    m_manager->unregisterProject(this);

    delete m_rootNode;
}

QDir QmlProject::projectDir() const
{
    return QFileInfo(file()->fileName()).dir();
}

QString QmlProject::filesFileName() const
{ return m_fileName; }

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            line = line.trimmed();
            if (line.isEmpty())
                continue;

            lines.append(line);
        }
    }

    return lines;
}

void QmlProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        if (options & ProjectFile)
            delete m_projectItem.data();
        if (!m_projectItem) {
            QFile file(m_fileName);
            if (file.open(QFile::ReadOnly)) {
                QmlComponent *component = new QmlComponent(&m_engine, this);
                component->setData(file.readAll(), QUrl::fromLocalFile(m_fileName));
                if (component->isReady()
                    && qobject_cast<QmlProjectItem*>(component->create())) {
                    m_projectItem = qobject_cast<QmlProjectItem*>(component->create());
                    connect(m_projectItem.data(), SIGNAL(qmlFilesChanged()), this, SLOT(refreshFiles()));
                } else {
                    qWarning() << m_fileName << "is not valid qml file, falling back to old format ...";
                    m_files = convertToAbsoluteFiles(readLines(filesFileName()));
                    m_files.removeDuplicates();
                    m_modelManager->updateSourceFiles(m_files);
                }
            }
        }
        if (m_projectItem) {
            m_projectItem.data()->setSourceDirectory(projectDir().path());
            m_modelManager->updateSourceFiles(m_projectItem.data()->files());
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
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();
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

QStringList QmlProject::libraryPaths() const
{
    QStringList libraryPaths;
    if (m_projectItem)
        libraryPaths = m_projectItem.data()->libraryPaths();
    return libraryPaths;
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

void QmlProject::refreshFiles()
{
    refresh(Files);
}

QString QmlProject::displayName() const
{
    return m_projectName;
}

QString QmlProject::id() const
{
    return QLatin1String("QmlProjectManager.QmlProject");
}

Core::IFile *QmlProject::file() const
{
    return m_file;
}

Internal::Manager *QmlProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> QmlProject::dependsOn()
{
    return QList<Project *>();
}

bool QmlProject::isApplication() const
{
    return true;
}

ProjectExplorer::BuildConfigWidget *QmlProject::createConfigWidget()
{
    return 0;
}

QList<ProjectExplorer::BuildConfigWidget*> QmlProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildConfigWidget*>();
}

Internal::QmlProjectTargetFactory *QmlProject::targetFactory() const
{
    return m_targetFactory;
}

Internal::QmlProjectTarget *QmlProject::activeTarget() const
{
    return static_cast<Internal::QmlProjectTarget *>(Project::activeTarget());
}

Internal::QmlProjectNode *QmlProject::rootProjectNode() const
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

    if (targets().isEmpty()) {
        Internal::QmlProjectTarget *target(targetFactory()->create(this, QLatin1String(Constants::QML_VIEWER_TARGET_ID)));
        addTarget(target);
    }

    refresh(Everything);
    return true;
}

} // namespace QmlProjectManager

