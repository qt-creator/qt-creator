/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pythoneditorplugin.h"
#include "pythoneditor.h"
#include "pythoneditorconstants.h"
#include "tools/pythonhighlighter.h"

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
#include <projectexplorer/target.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <texteditor/texteditorconstants.h>

#include <utils/detailswidget.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QFormLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace PythonEditor::Constants;
using namespace Utils;

/*******************************************************************************
 * List of Python keywords (includes "print" that isn't keyword in python 3
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_KEYWORDS[] = {
    "and",
    "as",
    "assert",
    "break",
    "class",
    "continue",
    "def",
    "del",
    "elif",
    "else",
    "except",
    "exec",
    "finally",
    "for",
    "from",
    "global",
    "if",
    "import",
    "in",
    "is",
    "lambda",
    "not",
    "or",
    "pass",
    "print",
    "raise",
    "return",
    "try",
    "while",
    "with",
    "yield"
};

/*******************************************************************************
 * List of Python magic methods and attributes
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_MAGICS[] = {
    // ctor & dtor
    "__init__",
    "__del__",
    // string conversion functions
    "__str__",
    "__repr__",
    "__unicode__",
    // attribute access functions
    "__setattr__",
    "__getattr__",
    "__delattr__",
    // binary operators
    "__add__",
    "__sub__",
    "__mul__",
    "__truediv__",
    "__floordiv__",
    "__mod__",
    "__pow__",
    "__and__",
    "__or__",
    "__xor__",
    "__eq__",
    "__ne__",
    "__gt__",
    "__lt__",
    "__ge__",
    "__le__",
    "__lshift__",
    "__rshift__",
    "__contains__",
    // unary operators
    "__pos__",
    "__neg__",
    "__inv__",
    "__abs__",
    "__len__",
    // item operators like []
    "__getitem__",
    "__setitem__",
    "__delitem__",
    "__getslice__",
    "__setslice__",
    "__delslice__",
    // other functions
    "__cmp__",
    "__hash__",
    "__nonzero__",
    "__call__",
    "__iter__",
    "__reversed__",
    "__divmod__",
    "__int__",
    "__long__",
    "__float__",
    "__complex__",
    "__hex__",
    "__oct__",
    "__index__",
    "__copy__",
    "__deepcopy__",
    "__sizeof__",
    "__trunc__",
    "__format__",
    // magic attributes
    "__name__",
    "__module__",
    "__dict__",
    "__bases__",
    "__doc__"
};

/*******************************************************************************
 * List of python built-in functions and objects
 ******************************************************************************/
static const char *const LIST_OF_PYTHON_BUILTINS[] = {
    "range",
    "xrange",
    "int",
    "float",
    "long",
    "hex",
    "oct"
    "chr",
    "ord",
    "len",
    "abs",
    "None",
    "True",
    "False"
};

namespace PythonEditor {
namespace Internal {

const char PythonRunConfigurationPrefix[] = "PythonEditor.RunConfiguration.";
const char InterpreterKey[] = "PythonEditor.RunConfiguation.Interpreter";
const char MainScriptKey[] = "PythonEditor.RunConfiguation.MainScript";
const char ArgumentsKey[] = "PythonEditor.RunConfiguation.Arguments";
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
    PythonProjectManager() {}

    QString mimeType() const { return QLatin1String(PythonMimeType); }
    Project *openProject(const QString &fileName, QString *errorString);

    void registerProject(PythonProject *project) { m_projects.append(project); }
    void unregisterProject(PythonProject *project) { m_projects.removeAll(project); }

private:
    QList<PythonProject *> m_projects;
};

class PythonProject : public Project
{
public:
    PythonProject(PythonProjectManager *manager, const QString &filename);
    ~PythonProject();

    QString displayName() const { return m_projectName; }
    IDocument *document() const;
    IProjectManager *projectManager() const { return m_manager; }

    ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode) const { return m_files; }
    QStringList files() const { return m_files; }

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);
    void refresh();

protected:
    bool fromMap(const QVariantMap &map);

private:
    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parseProject();
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    PythonProjectManager *m_manager;
    QString m_projectFileName;
    QString m_projectName;
    PythonProjectFile *m_document;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;

    ProjectNode *m_rootNode;
};

class PythonProjectFile : public Core::IDocument
{
public:
    PythonProjectFile(PythonProject *parent, QString fileName)
        : IDocument(parent),
          m_project(parent)
    {
        setId("Generic.ProjectFile");
        setMimeType(QLatin1String(PythonMimeType));
        setFilePath(FileName::fromString(fileName));
    }

    bool save(QString *errorString, const QString &fileName, bool autoSave)
    {
        Q_UNUSED(errorString)
        Q_UNUSED(fileName)
        Q_UNUSED(autoSave)
        return false;
    }

    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }

    bool isModified() const { return false; }
    bool isSaveAsAllowed() const { return false; }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const
    {
        Q_UNUSED(state)
        Q_UNUSED(type)
        return BehaviorSilent;
    }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type)
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
    PythonProjectNode(PythonProject *project, Core::IDocument *projectFile);

    Core::IDocument *projectFile() const;
    QString projectFilePath() const;

    bool showInSimpleTree() const;

    QList<ProjectAction> supportedActions(Node *node) const;

    bool canAddSubProject(const QString &proFilePath) const;

    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    bool deleteFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    void refresh(QSet<QString> oldFileList = QSet<QString>());

private:
    typedef QHash<QString, FolderNode *> FolderByName;
    FolderNode *createFolderByName(const QStringList &components, int end);
    FolderNode *findFolderByName(const QStringList &components, int end);
    void removeEmptySubFolders(FolderNode *gparent, FolderNode *parent);

private:
    PythonProject *m_project;
    Core::IDocument *m_projectFile;
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
public:
    PythonRunConfiguration(Target *parent, Core::Id id);

    QWidget *createConfigurationWidget();
    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &map);
    bool isEnabled() const { return m_enabled; }
    QString disabledReason() const;

    QString mainScript() const { return m_mainScript; }
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

    void start();
    StopResult stop();
    bool isRunning() const { return m_running; }

private:
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus status);
    void slotAppendMessage(const QString &err, Utils::OutputFormat isError);

    ApplicationLauncher m_applicationLauncher;
    QString m_interpreter;
    QString m_mainScript;
    QString m_commandLineArguments;
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
    const QString exec = sysEnv.searchInPath(QLatin1String("python")).toString();
    m_interpreter = exec.isEmpty() ? QLatin1String("python") : exec;

    addExtraAspect(new LocalEnvironmentAspect(this));
    addExtraAspect(new ArgumentsAspect(this, QStringLiteral("PythonEditor.RunConfiguration.Arguments")));
    addExtraAspect(new TerminalAspect(this, QStringLiteral("PythonEditor.RunConfiguration.UseTerminal")));
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
    map.insert(QLatin1String(MainScriptKey), m_mainScript);
    map.insert(QLatin1String(InterpreterKey), m_interpreter);
    return map;
}

bool PythonRunConfiguration::fromMap(const QVariantMap &map)
{
    m_mainScript = map.value(QLatin1String(MainScriptKey)).toString();
    m_interpreter = map.value(QLatin1String(InterpreterKey)).toString();
    return RunConfiguration::fromMap(map);
}

QString PythonRunConfiguration::defaultDisplayName() const
{
    QString result = tr("Run %1").arg(m_mainScript);
    if (!m_enabled) {
        result += QLatin1Char(' ');
        result += tr("(disabled)");
    }
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
        setObjectName(QLatin1String("PythonRunConfigurationFactory"));
    }

    QList<Core::Id> availableCreationIds(Target *parent, CreationMode mode) const
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

    QString displayNameForId(Core::Id id) const
    {
        return scriptFromId(id);
    }

    bool canCreate(Target *parent, Core::Id id) const
    {
        if (!canHandle(parent))
            return false;
        PythonProject *project = static_cast<PythonProject *>(parent->project());
        return project->files().contains(scriptFromId(id));
    }

    bool canRestore(Target *parent, const QVariantMap &map) const
    {
        Q_UNUSED(parent);
        return idFromMap(map).name().startsWith(PythonRunConfigurationPrefix);
    }

    bool canClone(Target *parent, RunConfiguration *source) const
    {
        if (!canHandle(parent))
            return false;
        return source->id().name().startsWith(PythonRunConfigurationPrefix);
    }

    RunConfiguration *clone(Target *parent, RunConfiguration *source)
    {
        if (!canClone(parent, source))
            return 0;
        return new PythonRunConfiguration(parent, static_cast<PythonRunConfiguration*>(source));
    }

private:
    bool canHandle(Target *parent) const { return dynamic_cast<PythonProject *>(parent->project()); }

    RunConfiguration *doCreate(Target *parent, Core::Id id)
    {
        return new PythonRunConfiguration(parent, id);
    }

    RunConfiguration *doRestore(Target *parent, const QVariantMap &map)
    {
        Core::Id id(idFromMap(map));
        return new PythonRunConfiguration(parent, id);
    }
};


PythonProject::PythonProject(PythonProjectManager *manager, const QString &fileName)
    : m_manager(manager),
      m_projectFileName(fileName)
{
    setId(PythonProjectId);
    setProjectContext(Context(PythonProjectContext));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    QFileInfo fileInfo(m_projectFileName);

    m_projectName = fileInfo.completeBaseName();
    m_document = new PythonProjectFile(this, m_projectFileName);

    DocumentManager::addDocument(m_document);

    m_rootNode = new PythonProjectNode(this, m_document);

    m_manager->registerProject(this);
}

PythonProject::~PythonProject()
{
    m_manager->unregisterProject(this);

    delete m_rootNode;
}

IDocument *PythonProject::document() const
{
    return m_document;
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
    bool result = saveRawList(rawFileList, m_projectFileName);
//    refresh(PythonProject::Files);
    return result;
}

bool PythonProject::saveRawList(const QStringList &rawList, const QString &fileName)
{
    DocumentManager::expectFileChange(fileName);
    // Make sure we can open the file for writing
    FileSaver saver(fileName, QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        foreach (const QString &filePath, rawList)
            stream << filePath << QLatin1Char('\n');
        saver.setResult(&stream);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(fileName);
    return result;
}

bool PythonProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_projectFileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    QSet<QString> toAdd;

    foreach (const QString &filePath, filePaths) {
        QString directory = QFileInfo(filePath).absolutePath();
        if (!toAdd.contains(directory))
            toAdd << directory;
    }

    bool result = saveRawList(newList, m_projectFileName);
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
    QDir baseDir(QFileInfo(m_projectFileName).dir());
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
            QDir baseDir(QFileInfo(m_projectFileName).dir());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

void PythonProject::parseProject()
{
    m_rawListEntries.clear();
    m_rawFileList = readLines(m_projectFileName);
    m_rawFileList << FileName::fromString(m_projectFileName).fileName();
    m_files = processEntries(m_rawFileList, &m_rawListEntries);
    emit fileListChanged();
}

/**
 * @brief Provides displayName relative to project node
 */
class PythonFileNode : public ProjectExplorer::FileNode
{
public:
    PythonFileNode(const Utils::FileName &filePath, const QString &nodeDisplayName)
        : ProjectExplorer::FileNode(filePath, SourceType, false)
        , m_displayName(nodeDisplayName)
    {}

    QString displayName() const { return m_displayName; }
private:
    QString m_displayName;
};

void PythonProject::refresh()
{
    m_rootNode->removeFileNodes(m_rootNode->fileNodes());
    parseProject();

    QDir baseDir = FileName::fromString(m_projectFileName).toFileInfo().absoluteDir();

    QList<FileNode *> fileNodes;
    foreach (const QString &file, m_files) {
        QString displayName = baseDir.relativeFilePath(file);
        fileNodes.append(new PythonFileNode(FileName::fromString(file), displayName));
    }

    m_rootNode->addFileNodes(fileNodes);
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
    const QDir projectDir(QFileInfo(m_projectFileName).dir());

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

ProjectNode *PythonProject::rootProjectNode() const
{
    return m_rootNode;
}

bool PythonProject::fromMap(const QVariantMap &map)
{
    if (Project::fromMap(map, 0) != RestoreResult::Ok)
        return false;

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    refresh();

    QList<Target *> targetList = targets();
    foreach (Target *t, targetList) {
        foreach (const QString &file, m_files)
            t->addRunConfiguration(new PythonRunConfiguration(t, idFromScript(file)));
    }

    return true;
}

PythonProjectNode::PythonProjectNode(PythonProject *project, Core::IDocument *projectFile)
    : ProjectNode(projectFile->filePath())
    , m_project(project)
    , m_projectFile(projectFile)
{
    setDisplayName(projectFile->filePath().toFileInfo().completeBaseName());
}

Core::IDocument *PythonProjectNode::projectFile() const
{
    return m_projectFile;
}

QString PythonProjectNode::projectFilePath() const
{
    return m_projectFile->filePath().toString();
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
            if (relativeFilePath.endsWith(QLatin1Char('/')))
                relativeFilePath.chop(1);
        }

        filesInPath[relativeFilePath].append(absoluteFileName);
    }
    return filesInPath;
}

void PythonProjectNode::refresh(QSet<QString> oldFileList)
{
    typedef QHash<QString, QStringList> FilesInPathHash;
    typedef FilesInPathHash::ConstIterator FilesInPathHashConstIt;

    // Do those separately
    oldFileList.remove(m_project->projectFilePath().toString());

    QSet<QString> newFileList = m_project->files().toSet();
    newFileList.remove(m_project->projectFilePath().toString());

    QSet<QString> removed = oldFileList;
    removed.subtract(newFileList);
    QSet<QString> added = newFileList;
    added.subtract(oldFileList);

    QString baseDir = path().toFileInfo().absolutePath();
    FilesInPathHash filesInPaths = sortFilesIntoPaths(baseDir, added);

    FilesInPathHashConstIt cend = filesInPaths.constEnd();
    for (FilesInPathHashConstIt it = filesInPaths.constBegin(); it != cend; ++it) {
        const QString &filePath = it.key();
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());
        if (!folder)
            folder = createFolderByName(components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            FileType fileType = SourceType; // ### FIXME
            if (file.endsWith(QLatin1String(".qrc")))
                fileType = ResourceType;
            FileNode *fileNode = new FileNode(FileName::fromString(file),
                                              fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        folder->addFileNodes(fileNodes);
    }

    filesInPaths = sortFilesIntoPaths(baseDir, removed);
    cend = filesInPaths.constEnd();
    for (FilesInPathHashConstIt it = filesInPaths.constBegin(); it != cend; ++it) {
        const QString &filePath = it.key();
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            foreach (FileNode *fn, folder->fileNodes()) {
                if (fn->path().toString() == file)
                    fileNodes.append(fn);
            }
        }

        folder->removeFileNodes(fileNodes);
    }

    foreach (FolderNode *fn, subFolderNodes())
        removeEmptySubFolders(this, fn);

}

void PythonProjectNode::removeEmptySubFolders(FolderNode *gparent, FolderNode *parent)
{
    foreach (FolderNode *fn, parent->subFolderNodes())
        removeEmptySubFolders(parent, fn);

    if (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty())
        gparent->removeFolderNodes(QList<FolderNode*>() << parent);
}

FolderNode *PythonProjectNode::createFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    const QString component = components.at(end - 1);

    const FileName folderPath = path().parentDir().appendPath(folderName);
    FolderNode *folder = new FolderNode(folderPath);
    folder->setDisplayName(component);

    FolderNode *parent = findFolderByName(components, end - 1);
    if (!parent)
        parent = createFolderByName(components, end - 1);
    parent->addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

FolderNode *PythonProjectNode::findFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    FolderNode *parent = findFolderByName(components, end - 1);

    if (!parent)
        return 0;

    const QString baseDir = path().toFileInfo().path();
    foreach (FolderNode *fn, parent->subFolderNodes()) {
        if (fn->path().toString() == baseDir + QLatin1Char('/') + folderName)
            return fn;
    }
    return 0;
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

bool PythonProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool PythonProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool PythonProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool PythonProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool PythonProjectNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool PythonProjectNode::deleteFiles(const QStringList &filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool PythonProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}

// PythonRunControlFactory

class PythonRunControlFactory : public IRunControlFactory
{
public:
    bool canRun(RunConfiguration *runConfiguration, Core::Id mode) const;
    RunControl *create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage);
};

bool PythonRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    return mode == ProjectExplorer::Constants::NORMAL_RUN_MODE && dynamic_cast<PythonRunConfiguration *>(runConfiguration);
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
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
    EnvironmentAspect *environment = rc->extraAspect<EnvironmentAspect>();
    Utils::Environment env;
    if (environment)
        env = environment->environment();
    m_applicationLauncher.setEnvironment(env);

    m_interpreter = rc->interpreter();
    m_mainScript = rc->mainScript();
    m_runMode = rc->extraAspect<TerminalAspect>()->runMode();
    m_commandLineArguments = rc->extraAspect<ArgumentsAspect>()->arguments();

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
        appendMessage(tr("No Python interpreter specified.") + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        emit finished();
    }  else if (!QFileInfo::exists(m_interpreter)) {
        appendMessage(tr("Python interpreter %1 does not exist.").arg(QDir::toNativeSeparators(m_interpreter)) + QLatin1Char('\n'),
                      Utils::ErrorMessageFormat);
        emit finished();
    } else {
        m_running = true;
        QString msg = tr("Starting %1...").arg(QDir::toNativeSeparators(m_interpreter)) + QLatin1Char('\n');
        appendMessage(msg, Utils::NormalMessageFormat);

        QString args;
        QtcProcess::addArg(&args, m_mainScript);
        QtcProcess::addArgs(&args, m_commandLineArguments);
        m_applicationLauncher.start(m_runMode, m_interpreter, args);

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
    appendMessage(msg + QLatin1Char('\n'), Utils::NormalMessageFormat);
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

/// Copies identifiers from array to QSet
static void copyIdentifiers(const char * const words[], size_t bytesCount, QSet<QString> &result)
{
    const size_t count = bytesCount / sizeof(const char * const);
    for (size_t i = 0; i < count; ++i)
        result.insert(QLatin1String(words[i]));
}

PythonEditorPlugin::PythonEditorPlugin()
{
    m_instance = this;
    copyIdentifiers(LIST_OF_PYTHON_KEYWORDS, sizeof(LIST_OF_PYTHON_KEYWORDS), m_keywords);
    copyIdentifiers(LIST_OF_PYTHON_MAGICS, sizeof(LIST_OF_PYTHON_MAGICS), m_magics);
    copyIdentifiers(LIST_OF_PYTHON_BUILTINS, sizeof(LIST_OF_PYTHON_BUILTINS), m_builtins);
}

PythonEditorPlugin::~PythonEditorPlugin()
{
    m_instance = 0;
}

bool PythonEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    MimeDatabase::addMimeTypes(QLatin1String(":/pythoneditor/PythonEditor.mimetypes.xml"));

    addAutoReleasedObject(new PythonProjectManager);
    addAutoReleasedObject(new PythonEditorFactory);
    addAutoReleasedObject(new PythonRunConfigurationFactory);
    addAutoReleasedObject(new PythonRunControlFactory);

    // Initialize editor actions handler
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon = QIcon::fromTheme(QLatin1String(C_PY_MIME_ICON));
    if (!icon.isNull())
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, C_PY_MIMETYPE);

    return true;
}

QSet<QString> PythonEditorPlugin::keywords()
{
    return m_instance->m_keywords;
}

QSet<QString> PythonEditorPlugin::magics()
{
    return m_instance->m_magics;
}

QSet<QString> PythonEditorPlugin::builtins()
{
    return m_instance->m_builtins;
}

} // namespace Internal
} // namespace PythonEditor

#include "pythoneditorplugin.moc"
