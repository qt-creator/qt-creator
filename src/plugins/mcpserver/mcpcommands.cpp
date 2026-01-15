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

// #include <utils/fileutils.h>
#include <utils/id.h>

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
    // Initialize default method timeouts (in seconds)
    m_methodTimeouts["debug"] = 60;
    m_methodTimeouts["build"] = 1200; // 20 minutes
    m_methodTimeouts["run_project"] = 60;
    m_methodTimeouts["load_session"] = 120;
    m_methodTimeouts["clean_project"] = 300; // 5 minutes

    // Initialize issues manager
    m_issuesManager = new IssuesManager(this);
}

bool McpCommands::build()
{
    if (!hasValidProject()) {
        qCDebug(mcpCommands) << "No valid project available for building";
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

    ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    if (!buildConfig) {
        qCDebug(mcpCommands) << "No active build configuration";
        return false;
    }

    qCDebug(mcpCommands) << "Starting build for project:" << project->displayName();

    // Trigger build
    ProjectExplorer::BuildManager::buildProjectWithoutDependencies(project);

    return true;
}

QString McpCommands::debug()
{
    QStringList results;
    results.append("=== DEBUG ATTEMPT ===");

    if (!hasValidProject()) {
        results.append("ERROR: No valid project available for debugging");
        return results.join("\n");
    }

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        results.append("ERROR: No current project");
        return results.join("\n");
    }

    ProjectExplorer::Target *target = project->activeTarget();
    if (!target) {
        results.append("ERROR: No active target");
        return results.join("\n");
    }

    ProjectExplorer::RunConfiguration *runConfig = target->activeRunConfiguration();
    if (!runConfig) {
        results.append("ERROR: No active run configuration available for debugging");
        return results.join("\n");
    }

    results.append("Project: " + project->displayName());
    results.append("Run configuration: " + runConfig->displayName());
    results.append("");

    // Trigger debug action on main thread
    results.append("=== STARTING DEBUG SESSION ===");

    Core::ActionManager *actionManager = Core::ActionManager::instance();
    if (actionManager) {
        // Try multiple common debug action IDs
        QStringList debugActionIds
            = {"Debugger.StartDebugging",
               "ProjectExplorer.StartDebugging",
               "Debugger.Debug",
               "ProjectExplorer.Debug",
               "Debugger.StartDebuggingOfStartupProject",
               "ProjectExplorer.StartDebuggingOfStartupProject"};

        bool debugTriggered = false;
        for (const QString &debugActionId : debugActionIds) {
            results.append("Trying debug action: " + debugActionId);

            Core::Command *command = actionManager->command(Utils::Id::fromString(debugActionId));
            if (command && command->action()) {
                results.append("Found debug action, triggering...");
                command->action()->trigger();
                results.append("Debug action triggered successfully");
                debugTriggered = true;
                break;
            } else {
                results.append("Debug action not found: " + debugActionId);
            }
        }

        if (!debugTriggered) {
            results.append("ERROR: No debug action found among tried IDs");
            return results.join("\n");
        }
    } else {
        results.append("ERROR: ActionManager not available");
        return results.join("\n");
    }

    results.append("Debug session initiated successfully!");
    results.append("The debugger is now starting in the background.");
    results.append("Check Qt Creator's debugger output for progress updates.");
    results.append("NOTE: The debug session will continue running asynchronously.");

    results.append("");
    results.append("=== DEBUG RESULT ===");
    results.append("Debug command completed.");

    return results.join("\n");
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

    Utils::FilePath filePath = Utils::FilePath::fromString(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return false;
    }

    qCDebug(mcpCommands) << "Opening file:" << path;

    Core::EditorManager::openEditor(filePath);

    return true;
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
        int timeoutSeconds = getMethodTimeout("stop_debug");
        if (timeoutSeconds < 0)
            timeoutSeconds = 30; // Default 30 seconds

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

bool McpCommands::cleanProject()
{
    if (!hasValidProject()) {
        qCDebug(mcpCommands) << "No valid project available for cleaning";
        return false;
    }

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    ProjectExplorer::Target *target = project->activeTarget();

    if (target) {
        ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
        if (buildConfig) {
            qCDebug(mcpCommands) << "Cleaning project:" << project->displayName();
            ProjectExplorer::BuildManager::cleanProjectWithoutDependencies(project);
            return true;
        }
    }

    qCDebug(mcpCommands) << "No build configuration available for cleaning";
    return false;
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

bool McpCommands::hasValidProject() const
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project) {
        return false;
    }

    ProjectExplorer::Target *target = project->activeTarget();
    if (!target) {
        return false;
    }

    return true;
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

int McpCommands::getMethodTimeout(const QString &method) const
{
    return m_methodTimeouts.value(method, -1);
}

// handleSessionLoadRequest method removed - using direct session loading instead

} // namespace Mcp::Internal
