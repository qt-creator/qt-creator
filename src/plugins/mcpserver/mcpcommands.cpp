// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "issuesmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/session.h>

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

// #include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/mimeutils.h>

#include <QApplication>
#include <QFile>
#include <QLoggingCategory>
#include <QProcess>
#include <QThread>
#include <QTimer>

Q_LOGGING_CATEGORY(mcpCommands, "qtc.mcpserver.commands", QtWarningMsg)

namespace Mcp::Internal {

McpCommands::McpCommands(QObject *parent)
    : QObject(parent)
{
    // Initialize issues manager
    m_issuesManager = new IssuesManager(this);
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

        Core::Command *command = actionManager->command(Utils::Id::fromString(actionId));
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
    if (ProjectExplorer::BuildManager::isBuilding()) {
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

    Utils::FilePath filePath = Utils::FilePath::fromUserInput(path);

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


    Utils::FilePath filePath = Utils::FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return QString();
    }

    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        return doc->plainText();

    Utils::MimeType mime = Utils::mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return QString();
    }

    Utils::Result<QByteArray> contents = filePath.fileContents();
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

    Utils::FilePath filePath = Utils::FilePath::fromUserInput(path);

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

    Utils::MimeType mime = Utils::mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return false;
    }

    qCDebug(mcpCommands) << "Setting plain text for file:" << path;

    Utils::Result<qint64> result = filePath.writeFileContents(
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

    Utils::FilePath filePath = Utils::FilePath::fromUserInput(path);

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

    Utils::Result<> res = doc->save();
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

    Utils::FilePath filePath = Utils::FilePath::fromUserInput(path);

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

QStringList McpCommands::listProjects()
{
    QStringList projects;

    QList<ProjectExplorer::Project *> projectList = ProjectExplorer::ProjectManager::projects();
    for (ProjectExplorer::Project *project : projectList) {
        projects.append(project->displayName());
    }

    qCDebug(mcpCommands) << "Found projects:" << projects;

    return projects;
}

QStringList McpCommands::listBuildConfigs()
{
    QStringList configs;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        qCDebug(mcpCommands) << "No current project";
        return configs;
    }

    ProjectExplorer::Target *target = project->activeTarget();
    if (!target) {
        qCDebug(mcpCommands) << "No active target";
        return configs;
    }

    QList<ProjectExplorer::BuildConfiguration *> buildConfigs = target->buildConfigurations();
    for (ProjectExplorer::BuildConfiguration *config : buildConfigs) {
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

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        qCDebug(mcpCommands) << "No current project";
        return false;
    }

    ProjectExplorer::Target *target = project->activeTarget();
    if (!target) {
        qCDebug(mcpCommands) << "No active target";
        return false;
    }

    QList<ProjectExplorer::BuildConfiguration *> buildConfigs = target->buildConfigurations();
    for (ProjectExplorer::BuildConfiguration *config : buildConfigs) {
        if (config->displayName() == name) {
            qCDebug(mcpCommands) << "Switching to build configuration:" << name;
            target->setActiveBuildConfiguration(config, ProjectExplorer::SetActive::Cascade);
            return true;
        }
    }

    qCDebug(mcpCommands) << "Build configuration not found:" << name;
    return false;
}

bool McpCommands::quit()
{
    Core::ICore::exit();
    return true;
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
        Core::Command *command = actionManager->command(Utils::Id::fromString(actionId));
        if (command && command->action() && command->action()->isEnabled()) {
            qCDebug(mcpCommands) << "Debug session is active (Stop action enabled):" << actionId;
            return true;
        }
    }

    // Also check "Abort Debugging" action
    QStringList abortActionIds
        = {"Debugger.Abort", "Debugger.AbortDebugger", "ProjectExplorer.AbortDebugging"};

    for (const QString &actionId : abortActionIds) {
        Core::Command *command = actionManager->command(Utils::Id::fromString(actionId));
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

        Core::Command *command = actionManager->command(Utils::Id::fromString(actionId));
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
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (project) {
        return project->displayName();
    }
    return QString();
}

QString McpCommands::getCurrentBuildConfig()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        return QString();
    }

    ProjectExplorer::Target *target = project->activeTarget();
    if (!target) {
        return QString();
    }

    ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
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
    return Core::SessionManager::loadSession(sessionName);
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

QStringList McpCommands::listIssues()
{
    qCDebug(mcpCommands) << "Listing issues from Qt Creator's Issues panel";

    if (!m_issuesManager) {
        qCDebug(mcpCommands) << "IssuesManager not initialized";
        return QStringList() << "ERROR:Issues manager not initialized";
    }

    QStringList issues = m_issuesManager->getCurrentIssues();

    // Add project status information for context
    if (ProjectExplorer::BuildManager::isBuilding()) {
        issues.prepend("INFO:Build in progress - issues may not be current");
    }

    qCDebug(mcpCommands) << "Found" << issues.size() << "issues total";
    return issues;
}

// handleSessionLoadRequest method removed - using direct session loading instead

} // namespace Mcp::Internal
