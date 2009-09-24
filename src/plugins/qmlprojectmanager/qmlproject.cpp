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
#include "qmlprojectconstants.h"
#include "qmlmakestep.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <qmleditor/qmlmodelmanagerinterface.h>

#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>

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

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;
using namespace ProjectExplorer;

////////////////////////////////////////////////////////////////////////////////////
// QmlProject
////////////////////////////////////////////////////////////////////////////////////

QmlProject::QmlProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_modelManager(ExtensionSystem::PluginManager::instance()->getObject<QmlEditor::QmlModelManagerInterface>())
{
    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = new QmlProjectFile(this, fileName);
    m_rootNode = new QmlProjectNode(this, m_file);

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
        m_files = convertToAbsoluteFiles(readLines(filesFileName()));
        m_files.removeDuplicates();
        m_modelManager->updateSourceFiles(m_files);
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
{ return m_files; }

QString QmlProject::buildParser(BuildConfiguration *configuration) const
{
    Q_UNUSED(configuration)
    return QString();
}

QString QmlProject::name() const
{
    return m_projectName;
}

Core::IFile *QmlProject::file() const
{
    return m_file;
}

Manager *QmlProject::projectManager() const
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

bool QmlProject::hasBuildSettings() const
{
    return false;
}

ProjectExplorer::Environment QmlProject::environment(BuildConfiguration *configuration) const
{
    Q_UNUSED(configuration)
    return ProjectExplorer::Environment::systemEnvironment();
}

QString QmlProject::buildDirectory(BuildConfiguration *configuration) const
{
    Q_UNUSED(configuration)
    return QString();
}

ProjectExplorer::BuildConfigWidget *QmlProject::createConfigWidget()
{
    return 0;
}

QList<ProjectExplorer::BuildConfigWidget*> QmlProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildConfigWidget*>();
}

ProjectExplorer::IBuildConfigurationFactory *QmlProject::buildConfigurationFactory() const
{
    return 0;
}

QmlProjectNode *QmlProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList QmlProject::files(FilesMode) const
{
    return m_files;
}

QStringList QmlProject::targets() const
{
    QStringList targets;
    return targets;
}

QmlMakeStep *QmlProject::makeStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        if (QmlMakeStep *ms = qobject_cast<QmlMakeStep *>(bs))
            return ms;
    }
    return 0;
}

bool QmlProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);

    if (runConfigurations().isEmpty()) {
        QSharedPointer<QmlRunConfiguration> runConf(new QmlRunConfiguration(this));
        addRunConfiguration(runConf);
    }

    if (buildSteps().isEmpty()) {
        QmlMakeStep *makeStep = new QmlMakeStep(this);
        insertBuildStep(0, makeStep);
    }

    refresh(Everything);
    return true;
}

void QmlProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    Project::saveSettingsImpl(writer);
}

////////////////////////////////////////////////////////////////////////////////////
// QmlProjectFile
////////////////////////////////////////////////////////////////////////////////////

QmlProjectFile::QmlProjectFile(QmlProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{ }

QmlProjectFile::~QmlProjectFile()
{ }

bool QmlProjectFile::save(const QString &)
{
    return false;
}

QString QmlProjectFile::fileName() const
{
    return m_fileName;
}

QString QmlProjectFile::defaultPath() const
{
    return QString();
}

QString QmlProjectFile::suggestedFileName() const
{
    return QString();
}

QString QmlProjectFile::mimeType() const
{
    return Constants::QMLMIMETYPE;
}

bool QmlProjectFile::isModified() const
{
    return false;
}

bool QmlProjectFile::isReadOnly() const
{
    return true;
}

bool QmlProjectFile::isSaveAsAllowed() const
{
    return false;
}

void QmlProjectFile::modified(ReloadBehavior *)
{
}

QmlRunConfiguration::QmlRunConfiguration(QmlProject *pro)
    : ProjectExplorer::LocalApplicationRunConfiguration(pro),
      m_project(pro),
      m_type(Constants::QMLRUNCONFIGURATION)
{
    setName(tr("QML Viewer"));

    m_qmlViewer = Core::Utils::SynchronousProcess::locateBinary(QLatin1String("qmlviewer"));
}

QmlRunConfiguration::~QmlRunConfiguration()
{
}

QString QmlRunConfiguration::type() const
{
    return m_type;
}

QString QmlRunConfiguration::executable() const
{
    // No need to verify if the QML Viewer exists. The console will tell us anyway when we try to launch it.
    return m_qmlViewer;
}

QmlRunConfiguration::RunMode QmlRunConfiguration::runMode() const
{
    return Gui;
}

QString QmlRunConfiguration::workingDirectory() const
{
    QFileInfo projectFile(m_project->file()->fileName());
    return projectFile.absolutePath();
}

QStringList QmlRunConfiguration::commandLineArguments() const
{
    QStringList args;

    if (!m_qmlViewerArgs.isEmpty())
        args.append(m_qmlViewerArgs);

    const QString s = mainScript();
    if (! s.isEmpty())
        args.append(s);

    return args;
}

ProjectExplorer::Environment QmlRunConfiguration::environment() const
{
    return ProjectExplorer::Environment::systemEnvironment();
}

QString QmlRunConfiguration::dumperLibrary() const
{
    return QString();
}

QStringList QmlRunConfiguration::dumperLibraryLocations() const
{
    return QStringList();
}

QWidget *QmlRunConfiguration::configurationWidget()
{
    QWidget *config = new QWidget;
    QFormLayout *form = new QFormLayout(config);

    QComboBox *combo = new QComboBox;

    QDir projectDir = m_project->projectDir();
    QStringList files;

    files.append(tr("<Current File>"));

    int currentIndex = -1;

    foreach (const QString &fn, m_project->files()) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        QString fileName = projectDir.relativeFilePath(fn);
        if (fileName == m_scriptFile)
            currentIndex = files.size();

        files.append(fileName);
    }

    combo->addItems(files);
    if (currentIndex != -1)
        combo->setCurrentIndex(currentIndex);

    connect(combo, SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));

    Core::Utils::PathChooser *qmlViewer = new Core::Utils::PathChooser;
    qmlViewer->setExpectedKind(Core::Utils::PathChooser::Command);
    qmlViewer->setPath(executable());
    connect(qmlViewer, SIGNAL(changed(QString)), this, SLOT(onQmlViewerChanged()));

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onQmlViewerArgsChanged()));

    form->addRow(tr("QML Viewer"), qmlViewer);
    form->addRow(tr("QML Viewer arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML File:"), combo);

    return config;
}

QString QmlRunConfiguration::mainScript() const
{
    if (m_scriptFile.isEmpty() || m_scriptFile == tr("<Current File>")) {
        Core::EditorManager *editorManager = Core::ICore::instance()->editorManager();
        if (Core::IEditor *editor = editorManager->currentEditor()) {
            return editor->file()->fileName();
        }
    }

    return m_project->projectDir().absoluteFilePath(m_scriptFile);
}

void QmlRunConfiguration::setMainScript(const QString &scriptFile)
{
    m_scriptFile = scriptFile;
}

void QmlRunConfiguration::onQmlViewerChanged()
{
    if (Core::Utils::PathChooser *chooser = qobject_cast<Core::Utils::PathChooser *>(sender())) {
        m_qmlViewer = chooser->path();
    }
}

void QmlRunConfiguration::onQmlViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_qmlViewerArgs = lineEdit->text();
}

void QmlRunConfiguration::save(ProjectExplorer::PersistentSettingsWriter &writer) const
{
    ProjectExplorer::LocalApplicationRunConfiguration::save(writer);

    writer.saveValue(QLatin1String("qmlviewer"), m_qmlViewer);
    writer.saveValue(QLatin1String("qmlviewerargs"), m_qmlViewerArgs);
    writer.saveValue(QLatin1String("mainscript"), m_scriptFile);
}

void QmlRunConfiguration::restore(const ProjectExplorer::PersistentSettingsReader &reader)
{
    ProjectExplorer::LocalApplicationRunConfiguration::restore(reader);

    m_qmlViewer = reader.restoreValue(QLatin1String("qmlviewer")).toString();
    m_qmlViewerArgs = reader.restoreValue(QLatin1String("qmlviewerargs")).toString();
    m_scriptFile = reader.restoreValue(QLatin1String("mainscript")).toString();

    if (m_qmlViewer.isEmpty())
        m_qmlViewer = Core::Utils::SynchronousProcess::locateBinary(QLatin1String("qmlviewer"));

    if (m_scriptFile.isEmpty())
        m_scriptFile = tr("<Current File>");
}

QmlRunConfigurationFactory::QmlRunConfigurationFactory()
    : m_type(Constants::QMLRUNCONFIGURATION)
{
}

QmlRunConfigurationFactory::~QmlRunConfigurationFactory()
{
}

bool QmlRunConfigurationFactory::canRestore(const QString &type) const
{
    if (type.startsWith(m_type))
        return true;

    return false;
}

QStringList QmlRunConfigurationFactory::availableCreationTypes(ProjectExplorer::Project *) const
{
    return QStringList();
}

QString QmlRunConfigurationFactory::displayNameForType(const QString &type) const
{
    return type;
}

QSharedPointer<ProjectExplorer::RunConfiguration> QmlRunConfigurationFactory::create(ProjectExplorer::Project *project,
                                                                                     const QString &)
{
    QmlProject *pro = qobject_cast<QmlProject *>(project);
    QSharedPointer<ProjectExplorer::RunConfiguration> rc(new QmlRunConfiguration(pro));
    return rc;
}


