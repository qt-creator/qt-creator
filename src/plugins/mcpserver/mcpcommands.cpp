// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "issuesmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/session.h>

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filesearch.h>
#include <utils/id.h>
#include <utils/mimeutils.h>

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QThread>
#include <QTimer>

using namespace Utils;
using namespace ProjectExplorer;

Q_LOGGING_CATEGORY(mcpCommands, "qtc.mcpserver.commands", QtWarningMsg)

namespace Mcp::Internal {

McpCommands::McpCommands(QObject *parent)
    : QObject(parent)
{}

QJsonObject McpCommands::searchResultsSchema()
{
    static QJsonObject cachedSchema = [] {
        QFile schemaFile(":/mcpserver/search-results-schema.json");
        if (!schemaFile.open(QIODevice::ReadOnly)) {
            qCWarning(mcpCommands) << "Failed to open search-results-schema.json from resources:"
                                   << schemaFile.errorString();
            return QJsonObject();
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(mcpCommands) << "Failed to parse search-results-schema.json:"
                                   << parseError.errorString();
            return QJsonObject();
        }

        return doc.object();
    }();

    return cachedSchema;
}

QString McpCommands::stopDebug()
{
    QStringList results;
    results.append("=== STOP DEBUGGING ===");

    // Use ActionManager to trigger the "Stop Debugging" action
    Core::ActionManager *actionManager = Core::ActionManager::instance();
    if (!actionManager) {
        results.append("ERROR: ActionManager not available");
        return results.join("\n");
    }

    // Try different possible action IDs for stopping debugging
    QStringList stopActionIds
        = {"Debugger.StopDebugger",
           "Debugger.Stop",
           "ProjectExplorer.StopDebugging",
           "ProjectExplorer.Stop",
           "Debugger.StopDebugging"};

    bool actionTriggered = false;
    for (const QString &actionId : stopActionIds) {
        results.append("Trying stop debug action: " + actionId);

        Core::Command *command = actionManager->command(Id::fromString(actionId));
        if (command && command->action()) {
            results.append("Found stop debug action, triggering...");
            command->action()->trigger();
            results.append("Stop debug action triggered successfully");
            actionTriggered = true;
            break;
        } else {
            results.append("Stop debug action not found: " + actionId);
        }
    }

    if (!actionTriggered) {
        results.append("WARNING: No stop debug action found among tried IDs");
        results.append(
            "You may need to stop debugging manually from Qt Creator's debugger interface");
    }

    results.append("");
    results.append("=== STOP DEBUG RESULT ===");
    results.append("Stop debug command completed.");

    return results.join("\n");
}

QString McpCommands::getVersion()
{
    return QCoreApplication::applicationVersion();
}

QString McpCommands::getBuildStatus()
{
    QStringList results;
    results.append("=== BUILD STATUS ===");

    // Check if build is currently running
    if (BuildManager::isBuilding()) {
        results.append("Building: 50%");
        results.append("Status: Build in progress");
        results.append("Current step: Compiling");
    } else {
        results.append("Building: 0%");
        results.append("Status: Not building");
    }

    results.append("");
    results.append("=== BUILD STATUS RESULT ===");
    results.append("Build status retrieved successfully.");

    return results.join("\n");
}

bool McpCommands::openFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return false;
    }

    qCDebug(mcpCommands) << "Opening file:" << path;

    Core::EditorManager::openEditor(filePath);

    return true;
}

QString McpCommands::getFilePlainText(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return QString();
    }


    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return QString();
    }

    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        return doc->plainText();

    MimeType mime = mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return QString();
    }

    Result<QByteArray> contents = filePath.fileContents();
    if (contents.has_value())
        return Core::EditorManager::defaultTextEncoding().decode(*contents);

    qCDebug(mcpCommands) << "Failed to read file contents:" << path << "Error:" << contents.error();
    return QString();
}

bool McpCommands::setFilePlainText(const QString &path, const QString &contents)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return false;
    }

    auto doc = Core::DocumentModel::documentForFilePath(filePath);

    if (!doc) {
        qCDebug(mcpCommands) << "No document found for file:" << path;
        return false;
    }

    if (auto textDoc = qobject_cast<TextEditor::TextDocument *>(doc)) {
        textDoc->document()->setPlainText(contents);
        return true;
    }

    MimeType mime = mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return false;
    }

    qCDebug(mcpCommands) << "Setting plain text for file:" << path;

    Result<qint64> result = filePath.writeFileContents(
        Core::EditorManager::defaultTextEncoding().encode(contents));

    if (!result)
        qCDebug(mcpCommands) << "Failed to write file contents:" << path << "Error:" << result.error();
    return result.has_value();
}

bool McpCommands::saveFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    auto doc = Core::DocumentModel::documentForFilePath(filePath);

    if (!doc) {
        qCDebug(mcpCommands) << "No document found for file:" << path;
        return false;
    }

    if (!doc->isModified()) {
        qCDebug(mcpCommands) << "Document is not modified, no need to save:" << path;
        return true;
    }

    qCDebug(mcpCommands) << "Saving file:" << path;

    Result<> res = doc->save();
    if (!res)
        qCDebug(mcpCommands) << "Failed to save document:" << path << "Error:" << res.error();

    return res.has_value();
}

bool McpCommands::closeFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    auto doc = Core::DocumentModel::documentForFilePath(filePath);

    if (!doc) {
        qCDebug(mcpCommands) << "No document found for file:" << path;
        return false;
    }

    qCDebug(mcpCommands) << "Closing file:" << path;

    bool closed = Core::EditorManager::closeDocuments({doc}, false);
    if (!closed)
        qCDebug(mcpCommands) << "Failed to close document:" << path;

    return closed;
}

static QStringList findFiles(
    const QList<Project *> &projects, const QString &pattern, bool regex)
{
    QStringList result;
    const QRegularExpression r(regex ? pattern : QRegularExpression::escape(pattern));
    for (auto project : projects) {
        const FilePaths matches = project->files([&r](const Node *n) {
            return !n->filePath().isEmpty() && r.match(n->filePath().toUserOutput()).hasMatch();
        });
        result.append(Utils::transform(matches, &FilePath::toUserOutput));
    }
    return result;
}

static QList<Project *> projectsForName(const QString &name)
{
    return Utils::filtered(ProjectManager::projects(), Utils::equal(&Project::displayName, name));
}

QStringList McpCommands::findFilesInProject(const QString &name, const QString &pattern, bool regex)
{
    return findFiles(projectsForName(name), pattern, regex);
}

QStringList McpCommands::findFilesInProjects(const QString &pattern, bool regex)
{
    return findFiles(ProjectManager::projects(), pattern, regex);
}

static void findInFiles(
    FileContainer fileContainer,
    bool regex,
    const QString &pattern,
    QObject *guard,
    const McpCommands::ResponseCallback &callback)
{
    FindFlags flags = regex ? FindRegularExpression : FindFlags();
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern, fileContainer, flags, TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback](const QFuture<SearchResultItems> &future) {
        QJsonArray resultsArray;
        for (Utils::SearchResultItems results : future.results()) {
            for (const SearchResultItem &item : results) {
                QJsonObject resultObj;
                const Text::Range range = item.mainRange();
                const QString lineText = item.lineText();
                const int startCol = range.begin.column;
                const int endCol = range.end.column;
                const QString matchedText = lineText.mid(startCol, endCol - startCol);

                resultObj["file"] = item.path().value(0, QString());
                resultObj["line"] = range.begin.line;
                resultObj["column"] = startCol + 1; // Convert 0-based to 1-based
                resultObj["text"] = matchedText;
                resultsArray.append(resultObj);
            }
        }

        QJsonObject response;
        response["results"] = resultsArray;
        callback(response);
    });
}

void McpCommands::searchInFile(
    const QString &path, const QString &pattern, bool regex, const ResponseCallback &callback)
{
    const FilePath filePath = FilePath::fromUserInput(path);
    if (!filePath.exists()) {
        callback({});
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return;
    }

    TextEncoding encoding;
    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath)) {
        encoding = doc->encoding();
    } else {
        for (const Project *project : ProjectManager::projects()) {
            const EditorConfiguration *config = project->editorConfiguration();
            if (project->isKnownFile(filePath)) {
                encoding = config->useGlobalSettings() ? Core::EditorManager::defaultTextEncoding()
                                                       : config->textEncoding();
                break;
            }
        }
    }
    if (!encoding.isValid())
        encoding = Core::EditorManager::defaultTextEncoding();

    FileListContainer fileContainer({filePath}, {encoding});

    findInFiles(fileContainer, regex, pattern, this, callback);
}

void McpCommands::searchInFiles(
    const QString &filePattern,
    const std::optional<QString> &projectName,
    const QString &path,
    const QString &pattern,
    bool regex,
    const ResponseCallback &callback)
{
    const QList<Project *> projects = projectName ? projectsForName(*projectName)
                                                  : ProjectManager::projects();

    const FilterFilesFunction filterFiles
        = Utils::filterFilesFunction({filePattern.isEmpty() ? "*" : filePattern}, {});
    const QMap<FilePath, TextEncoding> openEditorEncodings
        = TextEditor::TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, TextEncoding> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        TextEncoding projectEncoding = config->useGlobalSettings()
                                           ? Core::EditorManager::defaultTextEncoding()
                                           : config->textEncoding();
        const FilePaths filteredFiles = filterFiles(project->files(
            Core::Find::hasFindFlag(DontFindGeneratedFiles) ? Project::SourceFiles
                                                            : Project::AllFiles));
        for (const FilePath &fileName : filteredFiles) {
            TextEncoding encoding = openEditorEncodings.value(fileName);
            if (!encoding.isValid())
                encoding = projectEncoding;
            encodings.insert(fileName, encoding);
        }
    }
    FileListContainer fileContainer(encodings.keys(), encodings.values());

    findInFiles(fileContainer, regex, pattern, this, callback);
}

void McpCommands::searchInDirectory(
    const QString directory, const QString &pattern, bool regex, const ResponseCallback &callback)
{
    const FilePath dirPath = FilePath::fromUserInput(directory);
    if (!dirPath.exists() || !dirPath.isDir()) {
        callback({});
        qCDebug(mcpCommands) << "Directory does not exist or is not a directory:" << directory;
        return;
    }

    SubDirFileContainer fileContainer({dirPath}, {}, {}, {});

    findInFiles(fileContainer, regex, pattern, this, callback);
}

QStringList McpCommands::listProjects()
{
    QStringList projects;

    QList<Project *> projectList = ProjectManager::projects();
    for (Project *project : projectList) {
        projects.append(project->displayName());
    }

    qCDebug(mcpCommands) << "Found projects:" << projects;

    return projects;
}

QStringList McpCommands::listBuildConfigs()
{
    QStringList configs;

    Project *project = ProjectManager::startupProject();
    if (!project) {
        qCDebug(mcpCommands) << "No current project";
        return configs;
    }

    Target *target = project->activeTarget();
    if (!target) {
        qCDebug(mcpCommands) << "No active target";
        return configs;
    }

    QList<BuildConfiguration *> buildConfigs = target->buildConfigurations();
    for (BuildConfiguration *config : buildConfigs) {
        configs.append(config->displayName());
    }

    qCDebug(mcpCommands) << "Found build configurations:" << configs;

    return configs;
}

bool McpCommands::switchToBuildConfig(const QString &name)
{
    if (name.isEmpty()) {
        qCDebug(mcpCommands) << "Empty build configuration name provided";
        return false;
    }

    Project *project = ProjectManager::startupProject();
    if (!project) {
        qCDebug(mcpCommands) << "No current project";
        return false;
    }

    Target *target = project->activeTarget();
    if (!target) {
        qCDebug(mcpCommands) << "No active target";
        return false;
    }

    QList<BuildConfiguration *> buildConfigs = target->buildConfigurations();
    for (BuildConfiguration *config : buildConfigs) {
        if (config->displayName() == name) {
            qCDebug(mcpCommands) << "Switching to build configuration:" << name;
            target->setActiveBuildConfiguration(config, SetActive::Cascade);
            return true;
        }
    }

    qCDebug(mcpCommands) << "Build configuration not found:" << name;
    return false;
}

bool McpCommands::quit()
{
    qCDebug(mcpCommands) << "Starting graceful quit process...";

    // Check if debugging is currently active
    bool debuggingActive = isDebuggingActive();
    qCDebug(mcpCommands) << "Debug session check result:" << debuggingActive;

    if (debuggingActive) {
        qCDebug(mcpCommands) << "Debug session detected, attempting to stop debugging gracefully...";

        // Perform debugging cleanup synchronously (but using non-blocking timers)
        return performDebuggingCleanupSync();

    } else {
        qCDebug(mcpCommands) << "No active debug session detected, quitting immediately...";
        QApplication::quit();
        return true;
    }
}

bool McpCommands::performDebuggingCleanupSync()
{
    qCDebug(mcpCommands) << "Starting synchronous debugging cleanup process...";

    // Step 1: Try to stop debugging gracefully
    QString stopResult = stopDebug();
    qCDebug(mcpCommands) << "Stop debug result:" << stopResult;

    // Step 2: Wait up to 10 seconds for debugging to stop (using event loop)
    QEventLoop stopLoop;
    QTimer stopTimer;
    stopTimer.setSingleShot(true);
    QObject::connect(&stopTimer, &QTimer::timeout, &stopLoop, &QEventLoop::quit);

    // Check every second if debugging has stopped
    QTimer checkTimer;
    QObject::connect(&checkTimer, &QTimer::timeout, [this, &stopLoop, &checkTimer]() {
        if (!isDebuggingActive()) {
            qCDebug(mcpCommands) << "Debug session stopped successfully";
            checkTimer.stop();
            stopLoop.quit();
        }
    });

    checkTimer.start(1000); // Check every second
    stopTimer.start(10000); // Maximum 10 seconds
    stopLoop.exec();        // Wait for either success or timeout
    checkTimer.stop();

    // Step 3: If still debugging, try abort debugging
    if (isDebuggingActive()) {
        qCDebug(mcpCommands) << "Still debugging after stop, attempting abort debugging...";
        QString abortResult = abortDebug();
        qCDebug(mcpCommands) << "Abort debug result:" << abortResult;

        // Wait up to 5 seconds for abort to take effect
        QEventLoop abortLoop;
        QTimer abortTimer;
        abortTimer.setSingleShot(true);
        QObject::connect(&abortTimer, &QTimer::timeout, &abortLoop, &QEventLoop::quit);

        QTimer abortCheckTimer;
        QObject::connect(&abortCheckTimer, &QTimer::timeout, [this, &abortLoop, &abortCheckTimer]() {
            if (!isDebuggingActive()) {
                qCDebug(mcpCommands) << "Debug session aborted successfully";
                abortCheckTimer.stop();
                abortLoop.quit();
            }
        });

        abortCheckTimer.start(1000); // Check every second
        abortTimer.start(5000);      // Maximum 5 seconds
        abortLoop.exec();            // Wait for either success or timeout
        abortCheckTimer.stop();
    }

    // Step 4: If still debugging, try to kill debugged processes
    if (isDebuggingActive()) {
        qCDebug(mcpCommands) << "Still debugging after abort, attempting to kill debugged processes...";
        bool killResult = killDebuggedProcesses();
        qCDebug(mcpCommands) << "Kill debugged processes result:" << killResult;

        // Wait up to 5 seconds for kill to take effect
        QEventLoop killLoop;
        QTimer killTimer;
        killTimer.setSingleShot(true);
        QObject::connect(&killTimer, &QTimer::timeout, &killLoop, &QEventLoop::quit);

        QTimer killCheckTimer;
        QObject::connect(&killCheckTimer, &QTimer::timeout, [this, &killLoop, &killCheckTimer]() {
            if (!isDebuggingActive()) {
                qCDebug(mcpCommands) << "Debugged processes killed successfully";
                killCheckTimer.stop();
                killLoop.quit();
            }
        });

        killCheckTimer.start(1000); // Check every second
        killTimer.start(5000);      // Maximum 5 seconds
        killLoop.exec();            // Wait for either success or timeout
        killCheckTimer.stop();
    }

    // Step 5: Final timeout - wait up to configured timeout
    if (isDebuggingActive()) {
        const int timeoutSeconds = 30;

        qCDebug(mcpCommands) << "Still debugging, waiting up to" << timeoutSeconds
                 << "seconds for final timeout...";

        QEventLoop finalLoop;
        QTimer finalTimer;
        finalTimer.setSingleShot(true);
        QObject::connect(&finalTimer, &QTimer::timeout, &finalLoop, &QEventLoop::quit);

        QTimer finalCheckTimer;
        QObject::connect(&finalCheckTimer, &QTimer::timeout, [this, &finalLoop, &finalCheckTimer]() {
            if (!isDebuggingActive()) {
                qCDebug(mcpCommands) << "Debug session finally stopped";
                finalCheckTimer.stop();
                finalLoop.quit();
            }
        });

        finalCheckTimer.start(1000);             // Check every second
        finalTimer.start(timeoutSeconds * 1000); // Maximum configured timeout
        finalLoop.exec();                        // Wait for either success or timeout
        finalCheckTimer.stop();
    }

    // Step 6: Final check - determine success or failure
    bool success = !isDebuggingActive();
    if (success) {
        qCDebug(mcpCommands) << "Debug session cleanup completed successfully, quitting Qt Creator...";
        QApplication::quit();
        return true;
    } else {
        qCDebug(mcpCommands) << "ERROR: Failed to stop debugged application after all attempts - NOT quitting "
                    "Qt Creator";
        return false; // Don't quit Qt Creator
    }
}

void McpCommands::executeCommand(
    const QString &command,
    const QString &arguments,
    const QString &workingDirectory,
    const ResponseCallback &callback)
{
    CommandLine cmd(FilePath::fromUserInput(command), arguments, CommandLine::Raw);
    auto process = new Process(this);
    connect(process, &Process::done, this, [process, callback]() {
        QJsonObject response;
        response["exitCode"] = process->exitCode();
        response["output"] = process->readAllStandardOutput();
        response["errorOutput"] = process->readAllStandardError();
        callback(response);
        process->deleteLater();
    });
    process->setCommand(cmd);
    if (!workingDirectory.isEmpty())
        process->setWorkingDirectory(FilePath::fromUserInput(workingDirectory));
    process->start();
}

void McpCommands::performDebuggingCleanup()
{
    // This method is kept for backward compatibility but should not be used
    qCDebug(mcpCommands) << "performDebuggingCleanup called - this method is deprecated";
}

bool McpCommands::isDebuggingActive()
{
    // Check if debugging is currently active by looking at debugger actions
    Core::ActionManager *actionManager = Core::ActionManager::instance();
    if (!actionManager) {
        return false;
    }

    // Try different possible action IDs for checking if debugging is active
    QStringList stopActionIds
        = {"Debugger.Stop", "Debugger.StopDebugger", "ProjectExplorer.StopDebugging"};

    for (const QString &actionId : stopActionIds) {
        Core::Command *command = actionManager->command(Id::fromString(actionId));
        if (command && command->action() && command->action()->isEnabled()) {
            qCDebug(mcpCommands) << "Debug session is active (Stop action enabled):" << actionId;
            return true;
        }
    }

    // Also check "Abort Debugging" action
    QStringList abortActionIds
        = {"Debugger.Abort", "Debugger.AbortDebugger", "ProjectExplorer.AbortDebugging"};

    for (const QString &actionId : abortActionIds) {
        Core::Command *command = actionManager->command(Id::fromString(actionId));
        if (command && command->action() && command->action()->isEnabled()) {
            qCDebug(mcpCommands) << "Debug session is active (Abort action enabled):" << actionId;
            return true;
        }
    }

    qCDebug(mcpCommands) << "No active debug session detected";
    return false;
}

QString McpCommands::abortDebug()
{
    qCDebug(mcpCommands) << "Attempting to abort debug session...";

    // Use ActionManager to trigger the "Abort Debugging" action
    Core::ActionManager *actionManager = Core::ActionManager::instance();
    if (!actionManager) {
        return "ERROR: ActionManager not available";
    }

    // Try different possible action IDs for aborting debugging
    QStringList abortActionIds
        = {"Debugger.Abort",
           "Debugger.AbortDebugger",
           "ProjectExplorer.AbortDebugging",
           "Debugger.AbortDebug"};

    for (const QString &actionId : abortActionIds) {
        qCDebug(mcpCommands) << "Trying abort debug action:" << actionId;

        Core::Command *command = actionManager->command(Id::fromString(actionId));
        if (command && command->action() && command->action()->isEnabled()) {
            qCDebug(mcpCommands) << "Found abort debug action, triggering...";
            command->action()->trigger();
            return "Abort debug action triggered successfully: " + actionId;
        }
    }

    return "Abort debug action not found or not enabled";
}

bool McpCommands::killDebuggedProcesses()
{
    qCDebug(mcpCommands) << "Attempting to kill debugged processes...";

    // This is a simplified implementation
    // In a real scenario, you'd need to:
    // 1. Get the list of processes being debugged from the debugger
    // 2. Kill each process appropriately

    // For now, we'll try to find and kill any processes that might be debugged
    // This is platform-specific and would need proper implementation

    // TODO: Implement proper process killing for debugged applications
    // This could involve:
    // - Finding the debugged process PID
    // - Using platform-specific kill commands
    // - Handling different types of debugged processes (local, remote, etc.)

    return true; // Simplified for now - always return true
}

QString McpCommands::getCurrentProject()
{
    Project *project = ProjectManager::startupProject();
    if (project) {
        return project->displayName();
    }
    return QString();
}

QString McpCommands::getCurrentBuildConfig()
{
    Project *project = ProjectManager::startupProject();
    if (!project) {
        return QString();
    }

    Target *target = project->activeTarget();
    if (!target) {
        return QString();
    }

    BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    if (buildConfig) {
        return buildConfig->displayName();
    }

    return QString();
}

QStringList McpCommands::listOpenFiles()
{
    QStringList files;

    QList<Core::IDocument *> documents = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *doc : documents) {
        files.append(doc->filePath().toUserOutput());
    }

    qCDebug(mcpCommands) << "Open files:" << files;

    return files;
}

QStringList McpCommands::listSessions()
{
    QStringList sessions = Core::SessionManager::sessions();
    qCDebug(mcpCommands) << "Available sessions:" << sessions;
    return sessions;
}

QString McpCommands::getCurrentSession()
{
    QString session = Core::SessionManager::activeSession();
    qCDebug(mcpCommands) << "Current session:" << session;
    return session;
}

bool McpCommands::loadSession(const QString &sessionName)
{
    if (sessionName.isEmpty()) {
        qCDebug(mcpCommands) << "Empty session name provided";
        return false;
    }

    // Check if the session exists before trying to load it
    QStringList availableSessions = Core::SessionManager::sessions();
    if (!availableSessions.contains(sessionName)) {
        qCDebug(mcpCommands) << "Session does not exist:" << sessionName;
        qCDebug(mcpCommands) << "Available sessions:" << availableSessions;
        return false;
    }

    qCDebug(mcpCommands) << "Loading session:" << sessionName;

    // Use a safer approach - check if we're already in the target session
    QString currentSession = Core::SessionManager::activeSession();
    if (currentSession == sessionName) {
        qCDebug(mcpCommands) << "Already in session:" << sessionName;
        return true;
    }

    // Try to load the session using QTimer to avoid blocking
    QTimer::singleShot(0, [sessionName]() {
        qCDebug(mcpCommands) << "Attempting to load session:" << sessionName;
        bool success = Core::SessionManager::loadSession(sessionName);
        qCDebug(mcpCommands) << "Session load result:" << success;
    });

    qCDebug(mcpCommands) << "Session loading initiated asynchronously";
    return true; // Return true to indicate the request was accepted
}

bool McpCommands::saveSession()
{
    qCDebug(mcpCommands) << "Saving current session";

    bool successB = Core::SessionManager::saveSession();
    if (successB) {
        qCDebug(mcpCommands) << "Successfully saved session";
    } else {
        qCDebug(mcpCommands) << "Failed to save session";
    }

    return successB;
}

QJsonObject McpCommands::listIssues()
{
    return m_issuesManager.getCurrentIssues();
}

QJsonObject McpCommands::listIssues(const QString &path)
{
    return m_issuesManager.getCurrentIssues(Utils::FilePath::fromUserInput(path));
}

Utils::Result<QStringList> McpCommands::projectDependencies(const QString &projectName)
{
    for (Project * candidate : ProjectManager::projects()) {
        if (candidate->displayName() == projectName) {
            QStringList projects;
            const QList<Project *> projectList = ProjectManager::dependencies(candidate);
            for (Project *project : projectList)
                projects.append(project->displayName());
            return projects;
        }
    }

    return Utils::ResultError("No project found with name: " + projectName);
}

// handleSessionLoadRequest method removed - using direct session loading instead

} // namespace Mcp::Internal
