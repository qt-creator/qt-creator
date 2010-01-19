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

#include <projectexplorer/toolchain.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/modemanager.h>

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
const char * const QML_DEBUG_SERVER_PORT_KEY("QmlProjectManager.QmlRunConfiguration.DebugServerPort");

const int DEFAULT_DEBUG_SERVER_PORT(3768);
} // namespace

////////////////////////////////////////////////////////////////////////////////////
// QmlProject
////////////////////////////////////////////////////////////////////////////////////

QmlProject::QmlProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_modelManager(ExtensionSystem::PluginManager::instance()->getObject<QmlJSEditor::QmlModelManagerInterface>()),
      m_fileWatcher(new ProjectExplorer::FileWatcher(this))
{
    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = new QmlProjectFile(this, fileName);
    m_rootNode = new QmlProjectNode(this, m_file);

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
    return files();
}

bool QmlProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);

    if (runConfigurations().isEmpty()) {
        QmlRunConfiguration *runConf = new QmlRunConfiguration(this);
        addRunConfiguration(runConf);
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

////////////////////////////////////////////////////////////////////////////////////
// QmlRunConfiguration
////////////////////////////////////////////////////////////////////////////////////

QmlRunConfiguration::QmlRunConfiguration(QmlProject *parent) :
    ProjectExplorer::RunConfiguration(parent, QLatin1String(QML_RC_ID)),
    m_debugServerPort(DEFAULT_DEBUG_SERVER_PORT)
{
    ctor();
}

QmlRunConfiguration::QmlRunConfiguration(QmlProject *parent, QmlRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_scriptFile(source->m_scriptFile),
    m_qmlViewerCustomPath(source->m_qmlViewerCustomPath),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_debugServerPort(source->m_debugServerPort)
{
    ctor();
}

void QmlRunConfiguration::ctor()
{
    setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));

    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::SynchronousProcess::pathSeparator()
                               + QString(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));
}

QmlRunConfiguration::~QmlRunConfiguration()
{
}

QmlProject *QmlRunConfiguration::qmlProject() const
{
    return static_cast<QmlProject *>(project());
}

QString QmlRunConfiguration::viewerPath() const
{
    if (!m_qmlViewerCustomPath.isEmpty())
        return m_qmlViewerCustomPath;
    return m_qmlViewerDefaultPath;
}

QStringList QmlRunConfiguration::viewerArguments() const
{
    QStringList args;

    if (!m_qmlViewerArgs.isEmpty())
        args.append(m_qmlViewerArgs);

    const QString s = mainScript();
    if (! s.isEmpty())
        args.append(s);
    return args;
}

QString QmlRunConfiguration::workingDirectory() const
{
    QFileInfo projectFile(qmlProject()->file()->fileName());
    return projectFile.absolutePath();
}

uint QmlRunConfiguration::debugServerPort() const
{
    return m_debugServerPort;
}

QWidget *QmlRunConfiguration::configurationWidget()
{
    QWidget *config = new QWidget;
    QFormLayout *form = new QFormLayout(config);

    QComboBox *combo = new QComboBox;

    QDir projectDir = qmlProject()->projectDir();
    QStringList files;

    files.append(tr("<Current File>"));

    int currentIndex = -1;

    foreach (const QString &fn, qmlProject()->files()) {
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

    Utils::PathChooser *qmlViewer = new Utils::PathChooser;
    qmlViewer->setExpectedKind(Utils::PathChooser::Command);
    qmlViewer->setPath(viewerPath());
    connect(qmlViewer, SIGNAL(changed(QString)), this, SLOT(onQmlViewerChanged()));

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onQmlViewerArgsChanged()));

    QSpinBox *debugPort = new QSpinBox;
    debugPort->setMinimum(1024); // valid registered/dynamic/free ports according to http://www.iana.org/assignments/port-numbers
    debugPort->setMaximum(65535);
    debugPort->setValue(m_debugServerPort);
    connect(debugPort, SIGNAL(valueChanged(int)), this, SLOT(onDebugServerPortChanged()));

    form->addRow(tr("QML Viewer"), qmlViewer);
    form->addRow(tr("QML Viewer arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML File:"), combo);
    form->addRow(tr("Debugging Port:"), debugPort);

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

    return qmlProject()->projectDir().absoluteFilePath(m_scriptFile);
}

void QmlRunConfiguration::setMainScript(const QString &scriptFile)
{
    m_scriptFile = scriptFile;
}

void QmlRunConfiguration::onQmlViewerChanged()
{
    if (Utils::PathChooser *chooser = qobject_cast<Utils::PathChooser *>(sender())) {
        m_qmlViewerCustomPath = chooser->path();
    }
}

void QmlRunConfiguration::onQmlViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_qmlViewerArgs = lineEdit->text();
}

void QmlRunConfiguration::onDebugServerPortChanged()
{
    if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(sender())) {
        m_debugServerPort = spinBox->value();
    }
}

QVariantMap QmlRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(QML_VIEWER_KEY), m_qmlViewerCustomPath);
    map.insert(QLatin1String(QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(QML_MAINSCRIPT_KEY),  m_scriptFile);
    map.insert(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), m_debugServerPort);
    return map;
}

bool QmlRunConfiguration::fromMap(const QVariantMap &map)
{
    m_qmlViewerCustomPath = map.value(QLatin1String(QML_VIEWER_KEY)).toString();
    m_qmlViewerArgs = map.value(QLatin1String(QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(QML_MAINSCRIPT_KEY), tr("<Current File>")).toString();
    m_debugServerPort = map.value(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), DEFAULT_DEBUG_SERVER_PORT).toUInt();

    return RunConfiguration::fromMap(map);
}

////////////////////////////////////////////////////////////////////////////////////
// QmlRunConfigurationFactory
////////////////////////////////////////////////////////////////////////////////////

QmlRunConfigurationFactory::QmlRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QmlRunConfigurationFactory::~QmlRunConfigurationFactory()
{
}

QStringList QmlRunConfigurationFactory::availableCreationIds(ProjectExplorer::Project *) const
{
    return QStringList() << QLatin1String(QML_RC_ID);
}

QString QmlRunConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(QML_RC_ID))
        return tr("Run QML Script");
    return QString();
}

bool QmlRunConfigurationFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<QmlProject *>(parent))
        return false;
    return id == QLatin1String(QML_RC_ID);
}

ProjectExplorer::RunConfiguration *QmlRunConfigurationFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    QmlProject *project(static_cast<QmlProject *>(parent));
    return new QmlRunConfiguration(project);
}

bool QmlRunConfigurationFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    QString id(idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::RunConfiguration *QmlRunConfigurationFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QmlProject *project(static_cast<QmlProject *>(parent));
    QmlRunConfiguration *rc(new QmlRunConfiguration(project));
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

bool QmlRunConfigurationFactory::canClone(ProjectExplorer::Project *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *QmlRunConfigurationFactory::clone(ProjectExplorer::Project *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QmlProject *project(static_cast<QmlProject *>(parent));
    return new QmlRunConfiguration(project, qobject_cast<QmlRunConfiguration *>(source));
}

////////////////////////////////////////////////////////////////////////////////////
// QmlRunControl
////////////////////////////////////////////////////////////////////////////////////

QmlRunControl::QmlRunControl(QmlRunConfiguration *runConfiguration, bool debugMode)
    : RunControl(runConfiguration), m_debugMode(debugMode)
{
    Environment environment = ProjectExplorer::Environment::systemEnvironment();
    if (debugMode)
        environment.set("QML_DEBUG_SERVER_PORT", QString::number(runConfiguration->debugServerPort()));

    m_applicationLauncher.setEnvironment(environment.toStringList());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    m_executable = runConfiguration->viewerPath();
    m_commandLineArguments = runConfiguration->viewerArguments();

    connect(&m_applicationLauncher, SIGNAL(applicationError(QString)),
            this, SLOT(slotError(QString)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString)),
            this, SLOT(slotAddToOutputWindow(QString)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
}

QmlRunControl::~QmlRunControl()
{
}

void QmlRunControl::start()
{
    m_applicationLauncher.start(ApplicationLauncher::Gui, m_executable, m_commandLineArguments);
    emit started();
    emit addToOutputWindow(this, tr("Starting %1...").arg(QDir::toNativeSeparators(m_executable)));
}

void QmlRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool QmlRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void QmlRunControl::slotBringApplicationToForeground(qint64 pid)
{
    bringApplicationToForeground(pid);
}

void QmlRunControl::slotError(const QString &err)
{
    emit error(this, err);
    emit finished();
}

void QmlRunControl::slotAddToOutputWindow(const QString &line)
{
    if (m_debugMode && line.startsWith("QmlDebugServer: Waiting for connection")) {
        Core::ICore *core = Core::ICore::instance();
        core->modeManager()->activateMode(QLatin1String("QML_INSPECT_MODE"));
    }

    emit addToOutputWindowInline(this, line);
}

void QmlRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(QDir::toNativeSeparators(m_executable)).arg(exitCode));
    emit finished();
}

QmlRunControlFactory::QmlRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

QmlRunControlFactory::~QmlRunControlFactory()
{

}

bool QmlRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    Q_UNUSED(mode);
    return (qobject_cast<QmlRunConfiguration*>(runConfiguration) != 0);
}

RunControl *QmlRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return new QmlRunControl(qobject_cast<QmlRunConfiguration *>(runConfiguration),
                             mode == ProjectExplorer::Constants::DEBUGMODE);
}

QString QmlRunControlFactory::displayName() const
{
    return tr("Run");
}

QWidget *QmlRunControlFactory::configurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration)
    return new QLabel("TODO add Configuration widget");
}
