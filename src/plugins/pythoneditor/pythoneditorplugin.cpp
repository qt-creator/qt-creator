/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "pythoneditorplugin.h"
#include "pythoneditor.h"
#include "pythoneditorconstants.h"
#include "pythonhighlighter.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QFormLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace PythonEditor::Constants;
using namespace Utils;

namespace PythonEditor {
namespace Internal {

const char PythonRunConfigurationPrefix[] = "PythonEditor.RunConfiguration.";
const char InterpreterKey[] = "PythonEditor.RunConfiguation.Interpreter";
const char MainScriptKey[] = "PythonEditor.RunConfiguation.MainScript";
const char PythonMimeType[] = "text/x-python-project"; // ### FIXME
const char PythonProjectId[] = "PythonProject";
const char PythonProjectContext[] = "PythonProjectContext";

class PythonRunConfiguration;
class PythonProjectFile;
class PythonProject;

static QString scriptFromId(Core::Id id)
{
    return id.suffixAfter(PythonRunConfigurationPrefix);
}

static Core::Id idFromScript(const QString &target)
{
    return Core::Id(PythonRunConfigurationPrefix).withSuffix(target);
}

class PythonProjectManager : public IProjectManager
{
    Q_OBJECT
public:
    QString mimeType() const override { return QLatin1String(PythonMimeType); }
    Project *openProject(const QString &fileName, QString *errorString) override;

    void registerProject(PythonProject *project) { m_projects.append(project); }
    void unregisterProject(PythonProject *project) { m_projects.removeAll(project); }

private:
    QList<PythonProject *> m_projects;
};

class PythonProject : public Project
{
public:
    PythonProject(PythonProjectManager *manager, const QString &filename);
    ~PythonProject() override;

    QString displayName() const override { return m_projectName; }
    PythonProjectManager *projectManager() const override;

    QStringList files(FilesMode) const override { return m_files; }
    QStringList files() const { return m_files; }

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);
    void refresh();

private:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parseProject();
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    QString m_projectName;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
};

class PythonProjectFile : public Core::IDocument
{
public:
    PythonProjectFile(PythonProject *parent, QString fileName) : m_project(parent)
    {
        setId("Generic.ProjectFile");
        setMimeType(PythonMimeType);
        setFilePath(FileName::fromString(fileName));
    }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override
    {
        Q_UNUSED(state)
        Q_UNUSED(type)
        return BehaviorSilent;
    }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override
    {
        Q_UNUSED(errorString)
        Q_UNUSED(flag)
        if (type == TypePermissions)
            return true;
        m_project->refresh();
        return true;
    }

private:
    PythonProject *m_project;
};

class PythonProjectNode : public ProjectNode
{
public:
    PythonProjectNode(PythonProject *project);

    bool showInSimpleTree() const override;

    QList<ProjectAction> supportedActions(Node *node) const override;

    QString addFileFilter() const override;

    bool renameFile(const QString &filePath, const QString &newFilePath) override;

private:
    PythonProject *m_project;
};

class PythonRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    PythonRunConfigurationWidget(PythonRunConfiguration *runConfiguration, QWidget *parent = 0);
    void setInterpreter(const QString &interpreter);

private:
    PythonRunConfiguration *m_runConfiguration;
    DetailsWidget *m_detailsContainer;
    FancyLineEdit *m_interpreterChooser;
    QLabel *m_scriptLabel;
};

class PythonRunConfiguration : public RunConfiguration
{
    Q_OBJECT

    Q_PROPERTY(bool supportsDebugger READ supportsDebugger)
    Q_PROPERTY(QString interpreter READ interpreter)
    Q_PROPERTY(QString mainScript READ mainScript)
    Q_PROPERTY(QString arguments READ arguments)

public:
    PythonRunConfiguration(Target *parent, Core::Id id);

    QWidget *createConfigurationWidget() override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;
    bool isEnabled() const override { return m_enabled; }
    QString disabledReason() const override;

    bool supportsDebugger() const { return true; }
    QString mainScript() const { return m_mainScript; }
    QString arguments() const;
    QString interpreter() const { return m_interpreter; }
    void setInterpreter(const QString &interpreter) { m_interpreter = interpreter; }
    void setEnabled(bool b);

private:
    friend class PythonRunConfigurationFactory;
    PythonRunConfiguration(Target *parent, PythonRunConfiguration *source);
    QString defaultDisplayName() const;

    QString m_interpreter;
    QString m_mainScript;
    bool m_enabled;
};

class PythonRunControl : public RunControl
{
    Q_OBJECT
public:
    PythonRunControl(PythonRunConfiguration *runConfiguration, Core::Id mode);

    void start() override;
    StopResult stop() override;
    bool isRunning() const override { return m_running; }

private:
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus status);
    void slotAppendMessage(const QString &err, Utils::OutputFormat isError);

    ApplicationLauncher m_applicationLauncher;
    QString m_interpreter;
    QString m_mainScript;
    QString m_commandLineArguments;
    Utils::Environment m_environment;
    ApplicationLauncher::Mode m_runMode;
    bool m_running;
};

////////////////////////////////////////////////////////////////

PythonRunConfiguration::PythonRunConfiguration(Target *parent, Core::Id id) :
    RunConfiguration(parent, id),
    m_mainScript(scriptFromId(id)),
    m_enabled(true)
{
    Environment sysEnv = Environment::systemEnvironment();
    const QString exec = sysEnv.searchInPath("python").toString();
    m_interpreter = exec.isEmpty() ? "python" : exec;

    addExtraAspect(new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier()));
    addExtraAspect(new ArgumentsAspect(this, "PythonEditor.RunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "PythonEditor.RunConfiguration.UseTerminal"));
    setDefaultDisplayName(defaultDisplayName());
}

PythonRunConfiguration::PythonRunConfiguration(Target *parent, PythonRunConfiguration *source) :
    RunConfiguration(parent, source),
    m_interpreter(source->interpreter()),
    m_mainScript(source->m_mainScript),
    m_enabled(source->m_enabled)
{
    setDefaultDisplayName(defaultDisplayName());
}

QVariantMap PythonRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(MainScriptKey, m_mainScript);
    map.insert(InterpreterKey, m_interpreter);
    return map;
}

bool PythonRunConfiguration::fromMap(const QVariantMap &map)
{
    m_mainScript = map.value(MainScriptKey).toString();
    m_interpreter = map.value(InterpreterKey).toString();
    return RunConfiguration::fromMap(map);
}

QString PythonRunConfiguration::defaultDisplayName() const
{
    QString result = tr("Run %1").arg(m_mainScript);
    if (!m_enabled)
        result += ' ' + tr("(disabled)");
    return result;
}

QWidget *PythonRunConfiguration::createConfigurationWidget()
{
    return new PythonRunConfigurationWidget(this);
}

void PythonRunConfiguration::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
    setDefaultDisplayName(defaultDisplayName());
}

QString PythonRunConfiguration::disabledReason() const
{
    if (!m_enabled)
        return tr("The script is currently disabled.");
    return QString();
}

QString PythonRunConfiguration::arguments() const
{
    auto aspect = extraAspect<ArgumentsAspect>();
    QTC_ASSERT(aspect, return QString());
    return aspect->arguments();
}

PythonRunConfigurationWidget::PythonRunConfigurationWidget(PythonRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent), m_runConfiguration(runConfiguration)
{
    auto fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_interpreterChooser = new FancyLineEdit(this);
    m_interpreterChooser->setText(runConfiguration->interpreter());
    connect(m_interpreterChooser, &QLineEdit::textChanged,
            this, &PythonRunConfigurationWidget::setInterpreter);

    m_scriptLabel = new QLabel(this);
    m_scriptLabel->setText(runConfiguration->mainScript());

    fl->addRow(tr("Interpreter: "), m_interpreterChooser);
    fl->addRow(tr("Script: "), m_scriptLabel);
    runConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, fl);
    runConfiguration->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, fl);

    m_detailsContainer = new DetailsWidget(this);
    m_detailsContainer->setState(DetailsWidget::NoSummary);

    auto details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    details->setLayout(fl);

    auto vbx = new QVBoxLayout(this);
    vbx->setMargin(0);
    vbx->addWidget(m_detailsContainer);

    setEnabled(runConfiguration->isEnabled());
}

Project *PythonProjectManager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file.")
                .arg(fileName);
        return 0;
    }

    return new PythonProject(this, fileName);
}

class PythonRunConfigurationFactory : public IRunConfigurationFactory
{
public:
    PythonRunConfigurationFactory()
    {
        setObjectName("PythonRunConfigurationFactory");
    }

    QList<Core::Id> availableCreationIds(Target *parent, CreationMode mode) const override
    {
        Q_UNUSED(mode);
        if (!canHandle(parent))
            return {};
        //return { Core::Id(PythonExecutableId) };

        PythonProject *project = static_cast<PythonProject *>(parent->project());
        QList<Core::Id> allIds;
        foreach (const QString &file, project->files())
            allIds.append(idFromScript(file));
        return allIds;
    }

    QString displayNameForId(Core::Id id) const override
    {
        return scriptFromId(id);
    }

    bool canCreate(Target *parent, Core::Id id) const override
    {
        if (!canHandle(parent))
            return false;
        PythonProject *project = static_cast<PythonProject *>(parent->project());
        return project->files().contains(scriptFromId(id));
    }

    bool canRestore(Target *parent, const QVariantMap &map) const override
    {
        Q_UNUSED(parent);
        return idFromMap(map).name().startsWith(PythonRunConfigurationPrefix);
    }

    bool canClone(Target *parent, RunConfiguration *source) const override
    {
        if (!canHandle(parent))
            return false;
        return source->id().name().startsWith(PythonRunConfigurationPrefix);
    }

    RunConfiguration *clone(Target *parent, RunConfiguration *source) override
    {
        if (!canClone(parent, source))
            return 0;
        return new PythonRunConfiguration(parent, static_cast<PythonRunConfiguration*>(source));
    }

private:
    bool canHandle(Target *parent) const { return dynamic_cast<PythonProject *>(parent->project()); }

    RunConfiguration *doCreate(Target *parent, Core::Id id) override
    {
        return new PythonRunConfiguration(parent, id);
    }

    RunConfiguration *doRestore(Target *parent, const QVariantMap &map) override
    {
        Core::Id id(idFromMap(map));
        return new PythonRunConfiguration(parent, id);
    }
};

PythonProject::PythonProject(PythonProjectManager *manager, const QString &fileName)
{
    setId(PythonProjectId);
    setProjectManager(manager);
    setDocument(new PythonProjectFile(this, fileName));
    DocumentManager::addDocument(document());
    setRootProjectNode(new PythonProjectNode(this));

    setProjectContext(Context(PythonProjectContext));
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    QFileInfo fileInfo = projectFilePath().toFileInfo();

    m_projectName = fileInfo.completeBaseName();

    projectManager()->registerProject(this);
}

PythonProject::~PythonProject()
{
    projectManager()->unregisterProject(this);
}

PythonProjectManager *PythonProject::projectManager() const
{
    return static_cast<PythonProjectManager *>(Project::projectManager());
}

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

            lines.append(line);
        }
    }

    return lines;
}

bool PythonProject::saveRawFileList(const QStringList &rawFileList)
{
    bool result = saveRawList(rawFileList, projectFilePath().toString());
//    refresh(PythonProject::Files);
    return result;
}

bool PythonProject::saveRawList(const QStringList &rawList, const QString &fileName)
{
    FileChangeBlocker changeGuarg(fileName);
    // Make sure we can open the file for writing
    FileSaver saver(fileName, QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        foreach (const QString &filePath, rawList)
            stream << filePath << '\n';
        saver.setResult(&stream);
    }
    bool result = saver.finalize(ICore::mainWindow());
    return result;
}

bool PythonProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(projectDirectory().toString());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    QSet<QString> toAdd;

    foreach (const QString &filePath, filePaths) {
        QString directory = QFileInfo(filePath).absolutePath();
        if (!toAdd.contains(directory))
            toAdd << directory;
    }

    bool result = saveRawList(newList, projectFilePath().toString());
    refresh();

    return result;
}

bool PythonProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

bool PythonProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(projectFilePath().toString());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool PythonProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        int index = newList.indexOf(i.value());
        if (index != -1) {
            QDir baseDir(projectFilePath().toString());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

void PythonProject::parseProject()
{
    m_rawListEntries.clear();
    m_rawFileList = readLines(projectFilePath().toString());
    m_rawFileList << projectFilePath().fileName();
    m_files = processEntries(m_rawFileList, &m_rawListEntries);
    emit fileListChanged();
}

/**
 * @brief Provides displayName relative to project node
 */
class PythonFileNode : public FileNode
{
public:
    PythonFileNode(const Utils::FileName &filePath, const QString &nodeDisplayName)
        : FileNode(filePath, FileType::Source, false)
        , m_displayName(nodeDisplayName)
    {}

    QString displayName() const override { return m_displayName; }
private:
    QString m_displayName;
};

void PythonProject::refresh()
{
    parseProject();

    QDir baseDir(projectDirectory().toString());

    QList<FileNode *> fileNodes
            = Utils::transform(m_files, [baseDir](const QString &f) -> FileNode * {
        const QString displayName = baseDir.relativeFilePath(f);
        return new PythonFileNode(FileName::fromString(f), displayName);
    });
    rootProjectNode()->buildTree(fileNodes);

    emit parsingFinished();
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList PythonProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(projectDirectory().toString());

    QFileInfo fileInfo;
    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        trimmedPath = FileName::fromUserInput(trimmedPath).toString();

        fileInfo.setFile(projectDir, trimmedPath);
        if (fileInfo.exists()) {
            const QString absPath = fileInfo.absoluteFilePath();
            absolutePaths.append(absPath);
            if (map)
                map->insert(absPath, trimmedPath);
        }
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

Project::RestoreResult PythonProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    Project::RestoreResult res = Project::fromMap(map, errorMessage);
    if (res == RestoreResult::Ok) {
        Kit *defaultKit = KitManager::defaultKit();
        if (!activeTarget() && defaultKit)
            addTarget(createTarget(defaultKit));

        refresh();

        QList<Target *> targetList = targets();
        foreach (Target *t, targetList) {
            const QList<RunConfiguration *> runConfigs = t->runConfigurations();
            foreach (const QString &file, m_files) {
                // skip the 'project' file
                if (file.endsWith(".pyqtc"))
                    continue;
                const Id id = idFromScript(file);
                bool alreadyPresent = false;
                foreach (RunConfiguration *runCfg, runConfigs) {
                    if (runCfg->id() == id) {
                        alreadyPresent = true;
                        break;
                    }
                }
                if (!alreadyPresent)
                    t->addRunConfiguration(new PythonRunConfiguration(t, id));
            }
        }
    }

    return res;
}

PythonProjectNode::PythonProjectNode(PythonProject *project)
    : ProjectNode(project->projectDirectory())
    , m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());
}

QHash<QString, QStringList> sortFilesIntoPaths(const QString &base, const QSet<QString> &files)
{
    QHash<QString, QStringList> filesInPath;
    const QDir baseDir(base);

    foreach (const QString &absoluteFileName, files) {
        QFileInfo fileInfo(absoluteFileName);
        FileName absoluteFilePath = FileName::fromString(fileInfo.path());
        QString relativeFilePath;

        if (absoluteFilePath.isChildOf(baseDir)) {
            relativeFilePath = absoluteFilePath.relativeChildPath(FileName::fromString(base)).toString();
        } else {
            // 'file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath.toString());
            if (relativeFilePath.endsWith('/'))
                relativeFilePath.chop(1);
        }

        filesInPath[relativeFilePath].append(absoluteFileName);
    }
    return filesInPath;
}

bool PythonProjectNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectAction> PythonProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    //return { AddNewFile, AddExistingFile, AddExistingDirectory, RemoveFile, Rename };
    return {};
}

QString PythonProjectNode::addFileFilter() const
{
    return QLatin1String("*.py");
}

bool PythonProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}

// PythonRunControlFactory

class PythonRunControlFactory : public IRunControlFactory
{
public:
    bool canRun(RunConfiguration *runConfiguration, Core::Id mode) const override;
    RunControl *create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage) override;
};

bool PythonRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    return mode == ProjectExplorer::Constants::NORMAL_RUN_MODE
        && dynamic_cast<PythonRunConfiguration *>(runConfiguration);
}

RunControl *PythonRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return new PythonRunControl(static_cast<PythonRunConfiguration *>(runConfiguration), mode);
}

// PythonRunControl

PythonRunControl::PythonRunControl(PythonRunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode), m_running(false)
{
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);

    m_interpreter = rc->interpreter();
    m_mainScript = rc->mainScript();
    m_runMode = rc->extraAspect<TerminalAspect>()->runMode();
    m_commandLineArguments = rc->extraAspect<ArgumentsAspect>()->arguments();
    m_environment = rc->extraAspect<EnvironmentAspect>()->environment();

    connect(&m_applicationLauncher, &ApplicationLauncher::appendMessage,
            this, &PythonRunControl::slotAppendMessage);
    connect(&m_applicationLauncher, &ApplicationLauncher::processStarted,
            this, &PythonRunControl::processStarted);
    connect(&m_applicationLauncher, &ApplicationLauncher::processExited,
            this, &PythonRunControl::processExited);
    connect(&m_applicationLauncher, &ApplicationLauncher::bringToForegroundRequested,
            this, &RunControl::bringApplicationToForeground);
}

void PythonRunControl::start()
{
    emit started();
    if (m_interpreter.isEmpty()) {
        appendMessage(tr("No Python interpreter specified.") + '\n', Utils::ErrorMessageFormat);
        emit finished();
    }  else if (!QFileInfo::exists(m_interpreter)) {
        appendMessage(tr("Python interpreter %1 does not exist.").arg(QDir::toNativeSeparators(m_interpreter)) + '\n',
                      Utils::ErrorMessageFormat);
        emit finished();
    } else {
        m_running = true;
        QString msg = tr("Starting %1...").arg(QDir::toNativeSeparators(m_interpreter)) + '\n';
        appendMessage(msg, Utils::NormalMessageFormat);

        StandardRunnable r;
        QtcProcess::addArg(&r.commandLineArguments, m_mainScript);
        QtcProcess::addArgs(&r.commandLineArguments, m_commandLineArguments);
        r.executable = m_interpreter;
        r.runMode = m_runMode;
        r.environment = m_environment;
        m_applicationLauncher.start(r);

        setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
    }
}

PythonRunControl::StopResult PythonRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

void PythonRunControl::slotAppendMessage(const QString &err, Utils::OutputFormat format)
{
    appendMessage(err, format);
}

void PythonRunControl::processStarted()
{
    // Console processes only know their pid after being started
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
}

void PythonRunControl::processExited(int exitCode, QProcess::ExitStatus status)
{
    m_running = false;
    setApplicationProcessHandle(ProcessHandle());
    QString msg;
    if (status == QProcess::CrashExit) {
        msg = tr("%1 crashed")
                .arg(QDir::toNativeSeparators(m_interpreter));
    } else {
        msg = tr("%1 exited with code %2")
                .arg(QDir::toNativeSeparators(m_interpreter)).arg(exitCode);
    }
    appendMessage(msg + '\n', Utils::NormalMessageFormat);
    emit finished();
}

void PythonRunConfigurationWidget::setInterpreter(const QString &interpreter)
{
    m_runConfiguration->setInterpreter(interpreter);
}

////////////////////////////////////////////////////////////////////////////////////
//
// PythonEditorPlugin
//
////////////////////////////////////////////////////////////////////////////////////

static PythonEditorPlugin *m_instance = 0;

PythonEditorPlugin::PythonEditorPlugin()
{
    m_instance = this;
}

PythonEditorPlugin::~PythonEditorPlugin()
{
    m_instance = 0;
}

bool PythonEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    MimeDatabase::addMimeTypes(":/pythoneditor/PythonEditor.mimetypes.xml");

    addAutoReleasedObject(new PythonProjectManager);
    addAutoReleasedObject(new PythonEditorFactory);
    addAutoReleasedObject(new PythonRunConfigurationFactory);
    addAutoReleasedObject(new PythonRunControlFactory);

    return true;
}

void PythonEditorPlugin::extensionsInitialized()
{
    // Initialize editor actions handler
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon = QIcon::fromTheme(C_PY_MIME_ICON);
    if (!icon.isNull())
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, C_PY_MIMETYPE);
}

} // namespace Internal
} // namespace PythonEditor

#include "pythoneditorplugin.moc"
