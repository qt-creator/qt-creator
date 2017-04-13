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
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QFormLayout>
#include <QRegExp>

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

static QString scriptFromId(Core::Id id)
{
    return id.suffixAfter(PythonRunConfigurationPrefix);
}

static Core::Id idFromScript(const QString &target)
{
    return Core::Id(PythonRunConfigurationPrefix).withSuffix(target);
}

class PythonProject : public Project
{
public:
    explicit PythonProject(const Utils::FileName &filename);

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

    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
};

class PythonProjectNode : public ProjectNode
{
public:
    PythonProjectNode(PythonProject *project);

    bool showInSimpleTree() const override;
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
    Runnable runnable() const override;

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

Runnable PythonRunConfiguration::runnable() const
{
    StandardRunnable r;
    QtcProcess::addArg(&r.commandLineArguments, m_mainScript);
    QtcProcess::addArgs(&r.commandLineArguments, extraAspect<ArgumentsAspect>()->arguments());
    r.executable = m_interpreter;
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<EnvironmentAspect>()->environment();
    return r;
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
        foreach (const QString &file, project->files(ProjectExplorer::Project::AllFiles))
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
        return project->files(ProjectExplorer::Project::AllFiles).contains(scriptFromId(id));
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

PythonProject::PythonProject(const FileName &fileName) :
    Project(Constants::C_PY_MIMETYPE, fileName, [this]() { refresh(); })
{
    setId(PythonProjectId);
    setProjectContext(Context(PythonProjectContext));
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());
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
    auto newRoot = new PythonProjectNode(this);
    for (const QString &f : m_files) {
        const QString displayName = baseDir.relativeFilePath(f);
        newRoot->addNestedNode(new PythonFileNode(FileName::fromString(f), displayName));
    }
    setRootProjectNode(newRoot);

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
    auto rc = dynamic_cast<PythonRunConfiguration *>(runConfiguration);
    return mode == ProjectExplorer::Constants::NORMAL_RUN_MODE && rc && !rc->interpreter().isEmpty();
}

RunControl *PythonRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    auto runControl = new RunControl(runConfiguration, mode);
    (void) new SimpleTargetRunner(runControl);
    return runControl;
}

// PythonRunConfigurationWidget

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

    ProjectManager::registerProjectType<PythonProject>(PythonMimeType);

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
