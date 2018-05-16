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

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QRegExp>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextCursor>

using namespace Core;
using namespace ProjectExplorer;
using namespace PythonEditor::Constants;
using namespace Utils;

namespace PythonEditor {
namespace Internal {

const char PythonMimeType[] = "text/x-python-project"; // ### FIXME
const char PythonProjectId[] = "PythonProject";
const char PythonErrorTaskCategory[] = "Task.Category.Python";

class PythonProject : public Project
{
    Q_OBJECT
public:
    explicit PythonProject(const Utils::FileName &filename);

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);
    void refresh(Target *target = nullptr);

    bool needsConfiguration() const final { return false; }
    bool needsBuildConfigurations() const final { return false; }

private:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;
    bool setupTarget(Target *t) override
    {
        refresh(t);
        return Project::setupTarget(t);
    }

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

static QTextCharFormat linkFormat(const QTextCharFormat &inputFormat, const QString &href)
{
    QTextCharFormat result = inputFormat;
    result.setForeground(creatorTheme()->color(Theme::TextColorLink));
    result.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    result.setAnchor(true);
    result.setAnchorHref(href);
    return result;
}

class PythonOutputFormatter : public OutputFormatter
{
public:
    PythonOutputFormatter(Project *)
        // Note that moc dislikes raw string literals.
        : filePattern("^(\\s*)(File \"([^\"]+)\", line (\\d+), .*$)")
    {
        TaskHub::clearTasks(PythonErrorTaskCategory);
    }

private:
    void appendMessage(const QString &text, OutputFormat format) final
    {
        const bool isTrace = (format == StdErrFormat
                              || format == StdErrFormatSameLine)
                          && (text.startsWith("Traceback (most recent call last):")
                              || text.startsWith("\nTraceback (most recent call last):"));

        if (!isTrace) {
            OutputFormatter::appendMessage(text, format);
            return;
        }

        const QTextCharFormat frm = charFormat(format);
        const Core::Id id(PythonErrorTaskCategory);
        QVector<Task> tasks;
        const QStringList lines = text.split('\n');
        unsigned taskId = unsigned(lines.size());

        for (const QString &line : lines) {
            const QRegularExpressionMatch match = filePattern.match(line);
            if (match.hasMatch()) {
                QTextCursor tc = plainTextEdit()->textCursor();
                tc.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                tc.insertText('\n' + match.captured(1));
                tc.insertText(match.captured(2), linkFormat(frm, match.captured(2)));

                const auto fileName = FileName::fromString(match.captured(3));
                const int lineNumber = match.capturedRef(4).toInt();
                Task task(Task::Warning,
                                           QString(), fileName, lineNumber, id);
                task.taskId = --taskId;
                tasks.append(task);
            } else {
                if (!tasks.isEmpty()) {
                    Task &task = tasks.back();
                    if (!task.description.isEmpty())
                        task.description += ' ';
                    task.description += line.trimmed();
                }
                OutputFormatter::appendMessage('\n' + line, format);
            }
        }
        if (!tasks.isEmpty()) {
            tasks.back().type = Task::Error;
            for (auto rit = tasks.crbegin(), rend = tasks.crend(); rit != rend; ++rit)
                TaskHub::addTask(*rit);
        }
    }

    void handleLink(const QString &href) final
    {
        const QRegularExpressionMatch match = filePattern.match(href);
        if (!match.hasMatch())
            return;
        const QString fileName = match.captured(3);
        const int lineNumber = match.capturedRef(4).toInt();
        Core::EditorManager::openEditorAt(fileName, lineNumber);
    }

    const QRegularExpression filePattern;
};

////////////////////////////////////////////////////////////////

class InterpreterAspect : public BaseStringAspect
{
    Q_OBJECT

public:
    explicit InterpreterAspect(RunConfiguration *rc) : BaseStringAspect(rc) {}
};

class MainScriptAspect : public BaseStringAspect
{
    Q_OBJECT

public:
    explicit MainScriptAspect(RunConfiguration *rc) : BaseStringAspect(rc) {}
};

class PythonRunConfiguration : public RunConfiguration
{
    Q_OBJECT

    Q_PROPERTY(bool supportsDebugger READ supportsDebugger)
    Q_PROPERTY(QString interpreter READ interpreter)
    Q_PROPERTY(QString mainScript READ mainScript)
    Q_PROPERTY(QString arguments READ arguments)

public:
    PythonRunConfiguration(Target *target, Core::Id id);

private:
    void doAdditionalSetup(const RunConfigurationCreationInfo &) final { updateTargetInformation(); }
    void fillConfigurationLayout(QFormLayout *layout) const final;
    Runnable runnable() const final;

    bool supportsDebugger() const { return true; }
    QString mainScript() const { return extraAspect<MainScriptAspect>()->value(); }
    QString arguments() const { return extraAspect<ArgumentsAspect>()->arguments(); }
    QString interpreter() const { return extraAspect<InterpreterAspect>()->value(); }

    void updateTargetInformation();
};

PythonRunConfiguration::PythonRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    const Environment sysEnv = Environment::systemEnvironment();
    const QString exec = sysEnv.searchInPath("python").toString();

    auto interpreterAspect = new InterpreterAspect(this);
    interpreterAspect->setSettingsKey("PythonEditor.RunConfiguation.Interpreter");
    interpreterAspect->setLabelText(tr("Interpreter:"));
    interpreterAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    interpreterAspect->setHistoryCompleter("PythonEditor.Interpreter.History");
    interpreterAspect->setValue(exec.isEmpty() ? "python" : exec);
    addExtraAspect(interpreterAspect);

    auto scriptAspect = new MainScriptAspect(this);
    scriptAspect->setSettingsKey("PythonEditor.RunConfiguation.Script");
    scriptAspect->setLabelText(tr("Script:"));
    scriptAspect->setDisplayStyle(BaseStringAspect::LabelDisplay);
    addExtraAspect(scriptAspect);

    addExtraAspect(new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier()));
    addExtraAspect(new ArgumentsAspect(this, "PythonEditor.RunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "PythonEditor.RunConfiguration.UseTerminal"));

    setOutputFormatter<PythonOutputFormatter>();

    connect(target, &Target::applicationTargetsChanged,
            this, &PythonRunConfiguration::updateTargetInformation);
    connect(target->project(), &Project::parsingFinished,
            this, &PythonRunConfiguration::updateTargetInformation);
}

void PythonRunConfiguration::updateTargetInformation()
{
    const BuildTargetInfo bti = buildTargetInfo();
    const QString script = bti.targetFilePath.toString();
    setDefaultDisplayName(tr("Run %1").arg(script));
    extraAspect<MainScriptAspect>()->setValue(script);
}

void PythonRunConfiguration::fillConfigurationLayout(QFormLayout *layout) const
{
    extraAspect<InterpreterAspect>()->addToConfigurationLayout(layout);
    extraAspect<MainScriptAspect>()->addToConfigurationLayout(layout);
    extraAspect<ArgumentsAspect>()->addToConfigurationLayout(layout);
    extraAspect<TerminalAspect>()->addToConfigurationLayout(layout);
}

Runnable PythonRunConfiguration::runnable() const
{
    Runnable r;
    QtcProcess::addArg(&r.commandLineArguments, mainScript());
    QtcProcess::addArgs(&r.commandLineArguments, extraAspect<ArgumentsAspect>()->arguments());
    r.executable = extraAspect<InterpreterAspect>()->value();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<EnvironmentAspect>()->environment();
    return r;
}

class PythonRunConfigurationFactory : public RunConfigurationFactory
{
public:
    PythonRunConfigurationFactory()
    {
        registerRunConfiguration<PythonRunConfiguration>("PythonEditor.RunConfiguration.");
        addSupportedProjectType(PythonProjectId);
    }
};

PythonProject::PythonProject(const FileName &fileName) :
    Project(Constants::C_PY_MIMETYPE, fileName, [this]() { refresh(); })
{
    setId(PythonProjectId);
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
    PythonFileNode(const Utils::FileName &filePath, const QString &nodeDisplayName,
                   FileType fileType = FileType::Source)
        : FileNode(filePath, fileType, false)
        , m_displayName(nodeDisplayName)
    {}

    QString displayName() const override { return m_displayName; }
private:
    QString m_displayName;
};

void PythonProject::refresh(Target *target)
{
    emitParsingStarted();
    parseProject();

    QDir baseDir(projectDirectory().toString());
    BuildTargetInfoList appTargets;
    auto newRoot = std::make_unique<PythonProjectNode>(this);
    for (const QString &f : m_files) {
        const QString displayName = baseDir.relativeFilePath(f);
        FileType fileType = f.endsWith(".pyqtc") ? FileType::Project : FileType::Source;
        newRoot->addNestedNode(std::make_unique<PythonFileNode>(FileName::fromString(f),
                                                                displayName, fileType));
        if (fileType == FileType::Source) {
            BuildTargetInfo bti;
            bti.buildKey = f;
            bti.targetFilePath = FileName::fromString(f);
            bti.projectFilePath = projectFilePath();
            appTargets.list.append(bti);
        }
    }
    setRootProjectNode(std::move(newRoot));

    if (!target)
        target = activeTarget();
    if (target)
        target->setApplicationTargets(appTargets);

    emitParsingFinished(true);
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
        refresh();

        Kit *defaultKit = KitManager::defaultKit();
        if (!activeTarget() && defaultKit)
            addTarget(createTarget(defaultKit));
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

////////////////////////////////////////////////////////////////////////////////////
//
// PythonEditorPlugin
//
////////////////////////////////////////////////////////////////////////////////////

class PythonEditorPluginPrivate
{
public:
    PythonEditorFactory editorFactory;
    PythonRunConfigurationFactory runConfigFactory;
};

PythonEditorPlugin::~PythonEditorPlugin()
{
    delete d;
}

bool PythonEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new PythonEditorPluginPrivate;

    ProjectManager::registerProjectType<PythonProject>(PythonMimeType);

    auto constraint = [](RunConfiguration *runConfiguration) {
        auto aspect = runConfiguration->extraAspect<InterpreterAspect>();
        return aspect && !aspect->value().isEmpty();
    };
    RunControl::registerWorker<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE, constraint);

    return true;
}

void PythonEditorPlugin::extensionsInitialized()
{
    // Initialize editor actions handler
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon = QIcon::fromTheme(C_PY_MIME_ICON);
    if (!icon.isNull())
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, C_PY_MIMETYPE);

    TaskHub::addCategory(PythonErrorTaskCategory, "Python", true);
}

} // namespace Internal
} // namespace PythonEditor

#include "pythoneditorplugin.moc"
