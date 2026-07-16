// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "issuesmanager.h"

#include "buildmanager.h"
#include "task.h"
#include "taskhub.h"

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/id.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaMethod>
#include <QMetaObject>

static Q_LOGGING_CATEGORY(mcpIssues, "qtc.mcpserver.issues", QtWarningMsg)

static constexpr const char s_issuesSchema[] = R"json(
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://qt.io/qtcreator/mcpserver/issues-manager-output.schema.json",
  "title": "Qt Creator Issues Manager Output",
  "description": "Schema for issues passed to the MCP server from the IssuesManager, representing the current state of tracked issues and connection status to Qt Creator internals.",
  "type": "object",
  "required": ["issues", "summary", "status"],
  "properties": {
    "issues": {
      "description": "Array of current issues/tasks tracked by Qt Creator",
      "type": "array",
      "items": {
        "type": "object",
        "required": ["type", "description"],
        "properties": {
          "type": {
            "description": "The severity/type of the issue",
            "type": "string",
            "enum": ["ERROR", "WARNING", "INFO"]
          },
          "description": {
            "description": "The issue description/message",
            "type": "string",
            "minLength": 1
          },
          "file": {
            "description": "File path where the issue occurred (optional)",
            "type": "string"
          },
          "line": {
            "description": "Line number where the issue occurred (optional, -1 or absent means not specified)",
            "type": "integer",
            "minimum": -1
          },
          "id": {
            "description": "Internal task ID (if available)",
            "type": "integer"
          }
        }
      }
    },
    "summary": {
      "description": "Summary statistics of all issues",
      "type": "object",
      "required": ["totalTasks", "errorCount", "warningCount", "otherCount"],
      "properties": {
        "totalTasks": {
          "description": "Total number of tracked tasks",
          "type": "integer",
          "minimum": 0
        },
        "errorCount": {
          "description": "Number of error-level issues",
          "type": "integer",
          "minimum": 0
        },
        "warningCount": {
          "description": "Number of warning-level issues",
          "type": "integer",
          "minimum": 0
        },
        "otherCount": {
          "description": "Number of info/other-level issues",
          "type": "integer",
          "minimum": 0
        },
        "buildManagerErrorCount": {
          "description": "Error count from BuildManager (fallback when no tasks tracked)",
          "type": "integer",
          "minimum": 0
        }
      }
    },
    "status": {
      "description": "Status of the IssuesManager's connection to Qt Creator internals",
      "type": "object",
      "required": ["accessible", "signalsConnected", "taskWindowFound"],
      "properties": {
        "accessible": {
          "description": "Whether the Issues panel is accessible",
          "type": "boolean"
        },
        "signalsConnected": {
          "description": "Whether TaskHub signals are connected",
          "type": "boolean"
        },
        "taskWindowFound": {
          "description": "Whether the TaskWindow object was found",
          "type": "boolean"
        }
      }
    },
    "errorMessage": {
      "description": "Error message if the Issues panel is not accessible",
      "type": "string"
    }
  },
  "examples": [
    {
      "issues": [
        {
          "type": "ERROR",
          "description": "undefined reference to 'MyClass::myMethod()'",
          "file": "/path/to/project/main.cpp",
          "line": 42
        },
        {
          "type": "WARNING",
          "description": "unused variable 'foo'",
          "file": "/path/to/project/utils.cpp",
          "line": 15
        },
        {
          "type": "INFO",
          "description": "Build completed",
          "file": "",
          "line": -1
        }
      ],
      "summary": {
        "totalTasks": 3,
        "errorCount": 1,
        "warningCount": 1,
        "otherCount": 1
      },
      "status": {
        "accessible": true,
        "signalsConnected": true,
        "taskWindowFound": true
      }
    },
    {
      "issues": [],
      "summary": {
        "totalTasks": 0,
        "errorCount": 0,
        "warningCount": 0,
        "otherCount": 0,
        "buildManagerErrorCount": 0
      },
      "status": {
        "accessible": true,
        "signalsConnected": true,
        "taskWindowFound": true
      }
    },
    {
      "issues": [],
      "summary": {
        "totalTasks": 0,
        "errorCount": 0,
        "warningCount": 0,
        "otherCount": 0
      },
      "status": {
        "accessible": false,
        "signalsConnected": false,
        "taskWindowFound": false
      },
      "errorMessage": "ERROR:Issues panel not accessible - cannot retrieve current issues"
    }
  ]
}
)json";

namespace ProjectExplorer {

IssuesManager::IssuesManager(QObject *parent)
    : QObject(parent)
{
    initializeAccess();
    connectSignals();
}

QJsonObject IssuesManager::getCurrentIssues() const
{
    return getCurrentIssues([](const ProjectExplorer::Task &) { return true; });
}

QJsonObject IssuesManager::getCurrentIssues(const Utils::FilePath &path) const
{
    return getCurrentIssues(Utils::equal(&ProjectExplorer::Task::file, path));
}

Mcp::Schema::Tool::OutputSchema IssuesManager::issuesSchema()
{
    static Mcp::Schema::Tool::OutputSchema cachedSchema = [] {

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(s_issuesSchema, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(mcpIssues) << "Failed to parse issues-schema:"
                                 << parseError.errorString();
            return Mcp::Schema::Tool::OutputSchema{};
        }

        QJsonObject obj = doc.object();
        Mcp::Schema::Tool::OutputSchema schema;
        if (obj.contains("required") && obj["required"].isArray()) {
            QStringList req;
            for (const QJsonValue &v : obj["required"].toArray())
                req.append(v.toString());
            schema._required = req;
        }
        return schema;
    }();
    return cachedSchema;
}

bool IssuesManager::initializeAccess()
{
    // Check if we can access the BuildManager
    if (ProjectExplorer::BuildManager::instance()) {
        m_accessible = true;
        qCDebug(mcpIssues) << "IssuesManager: Successfully initialized with BuildManager access";

        // Find and store the TaskWindow object
        const QObjectList allObjects = ExtensionSystem::PluginManager::allObjects();
        for (QObject *obj : allObjects) {
            if (obj && QString::fromLatin1(obj->metaObject()->className()).contains("TaskWindow")) {
                m_taskWindow = obj;
                qCDebug(mcpIssues) << "IssuesManager: Found TaskWindow object";
                break;
            }
        }

        return true;
    }

    qCDebug(mcpIssues) << "IssuesManager: Failed to initialize - BuildManager not accessible";
    return false;
}

QJsonObject IssuesManager::getCurrentIssues(
    std::function<bool(const ProjectExplorer::Task &)> filter) const
{
    QJsonObject result;

    // Check accessibility first
    if (!m_accessible) {
        result["errorMessage"] = QString("Issues panel not accessible - cannot retrieve current issues");
        result["issues"] = QJsonArray();
        result["summary"] = QJsonObject{
            {"totalTasks", 0},
            {"errorCount", 0},
            {"warningCount", 0},
            {"otherCount", 0}
        };
        result["status"] = QJsonObject{
            {"accessible", false},
            {"signalsConnected", false},
            {"taskWindowFound", false},
            {"buildManagerAvailable", false}
        };
        return result;
    }

    // Build issues array
    QJsonArray issuesArray;
    int taskCount = 0;
    int errorCount = 0;
    int warningCount = 0;
    int otherCount = 0;

    for (const ProjectExplorer::Task &task : m_trackedTasks) {
        if (filter && !filter(task))
            continue; // Skip tasks that don't match the filter
        ++taskCount;
        QJsonObject issueObj;

        // Determine type
        QString taskType;
        if (task.type() == ProjectExplorer::Task::Error) {
            taskType = "ERROR";
            errorCount++;
        } else if (task.type() == ProjectExplorer::Task::Warning) {
            taskType = "WARNING";
            warningCount++;
        } else {
            taskType = "INFO";
            otherCount++;
        }

        issueObj["type"] = taskType;
        issueObj["description"] = task.description();

        // Add optional fields
        if (!task.file().isEmpty())
            issueObj["file"] = task.file().toUserOutput();

        if (task.line() > 0)
            issueObj["line"] = task.line();

        issueObj["id"] = static_cast<int>(task.id());

        issuesArray.append(issueObj);
    }

    // Build summary object
    QJsonObject summary;
    summary["totalTasks"] = taskCount;
    summary["errorCount"] = errorCount;
    summary["warningCount"] = warningCount;
    summary["otherCount"] = otherCount;

    // Add buildManagerErrorCount if no tasks tracked but BuildManager has data
    if (m_trackedTasks.isEmpty() && ProjectExplorer::BuildManager::tasksAvailable())
        summary["buildManagerErrorCount"] = ProjectExplorer::BuildManager::getErrorTaskCount();

    // Build status object
    QJsonObject status;
    status["accessible"] = m_accessible;
    status["signalsConnected"] = m_signalsConnected;
    status["taskWindowFound"] = m_taskWindow != nullptr;
    status["buildManagerAvailable"] = ProjectExplorer::BuildManager::tasksAvailable();

    // Assemble final result
    result["issues"] = issuesArray;
    result["summary"] = summary;
    result["status"] = status;

    return result;
}

void IssuesManager::connectSignals()
{
    if (m_signalsConnected) {
        return;
    }

    // Connect to TaskHub signals
    try {
        ProjectExplorer::TaskHub &hub = ProjectExplorer::taskHub();

        // Connect to task added/removed signals
        connect(&hub, &ProjectExplorer::TaskHub::taskAdded, this, &IssuesManager::onTaskAdded);
        connect(&hub, &ProjectExplorer::TaskHub::taskRemoved, this, &IssuesManager::onTaskRemoved);
        connect(&hub, &ProjectExplorer::TaskHub::tasksCleared, this, &IssuesManager::onTasksCleared);

        qCDebug(mcpIssues) << "IssuesManager: Connected to TaskHub signals";

        // Connect to TaskWindow signals if available
        if (m_taskWindow) {
            connect(m_taskWindow, SIGNAL(tasksChanged()), this, SLOT(onTasksChanged()));
            qCDebug(mcpIssues) << "IssuesManager: Connected to TaskWindow tasksChanged signal";
        }

        m_signalsConnected = true;
    } catch (...) {
        qCDebug(mcpIssues) << "IssuesManager: Failed to connect to TaskHub signals";
    }
}

void IssuesManager::onTaskAdded(const ProjectExplorer::Task &task)
{
    qCDebug(mcpIssues) << "IssuesManager: Task added:" << task.description();
    m_trackedTasks.append(task);
}

void IssuesManager::onTaskRemoved(const ProjectExplorer::Task &task)
{
    qCDebug(mcpIssues) << "IssuesManager: Task removed:" << task.description();
    m_trackedTasks.removeIf(Utils::equal(&ProjectExplorer::Task::id, task.id()));
}

void IssuesManager::onTasksCleared(const Utils::Id &category)
{
    qCDebug(mcpIssues) << "IssuesManager: Tasks cleared:" << category.toString();
    if (!category.isValid()) { // invalid Id means "clear all" (TaskHub::clearTasks() with no arg)
        m_trackedTasks.clear();
        return;
    }
    m_trackedTasks.removeIf(Utils::equal(&ProjectExplorer::Task::category, category));
}

void IssuesManager::onTasksChanged()
{
    qCDebug(mcpIssues) << "IssuesManager: TaskWindow reports tasks changed";
}

QStringList IssuesManager::testTaskAccess() const
{
    QStringList results;
    results.append("=== COMPREHENSIVE TASK ACCESS TEST ===");

    // Test 1: Try to access TaskWindow through PluginManager
    results.append("");
    results.append("Test 1: ExtensionSystem::PluginManager::getObject<TaskWindow>");
    results.append("SKIPPED: TaskWindow is internal class, symbols not exported");

    // Test 2: Try to access TaskWindow through IOutputPane
    results.append("");
    results.append("Test 2: Core::IOutputPane access");
    try {
        auto *outputPane = ExtensionSystem::PluginManager::getObject<Core::IOutputPane>();
        if (outputPane) {
            results.append("SUCCESS: IOutputPane found");
            results.append(QString("IOutputPane class: %1")
                               .arg(QString::fromUtf8(outputPane->metaObject()->className())));
        } else {
            results.append("FAILED: IOutputPane not found");
        }
    } catch (...) {
        results.append("EXCEPTION: Error accessing IOutputPane");
    }

    // Test 3: Try to access TaskHub
    results.append("");
    results.append("Test 3: TaskHub access");
    try {
        ProjectExplorer::TaskHub &hub = ProjectExplorer::taskHub();
        results.append("SUCCESS: TaskHub reference obtained");
        results.append(QString("TaskHub object: %1").arg(reinterpret_cast<quintptr>(&hub), 0, 16));

        // Check if TaskHub has any useful methods we can call
        const QMetaObject *metaObj = hub.metaObject();
        results.append(QString("TaskHub class: %1").arg(QString::fromUtf8(metaObj->className())));
        results.append("TaskHub methods:");
        for (int i = 0; i < metaObj->methodCount(); ++i) {
            QMetaMethod method = metaObj->method(i);
            if (method.access() == QMetaMethod::Public) {
                results.append(QString("  - %1").arg(QString::fromLatin1(method.methodSignature())));
            }
        }
    } catch (...) {
        results.append("EXCEPTION: Error accessing TaskHub");
    }

    // Test 4: Try to access TaskModel through PluginManager
    results.append("");
    results.append("Test 4: TaskModel access via PluginManager");
    results.append("SKIPPED: TaskModel is internal class, symbols not exported");

    // Test 5: BuildManager task information
    results.append("");
    results.append("Test 5: BuildManager task information");
    try {
        bool tasksAvailable = ProjectExplorer::BuildManager::tasksAvailable();
        int errorCount = ProjectExplorer::BuildManager::getErrorTaskCount();
        results.append(QString("BuildManager tasks available: %1")
                           .arg(tasksAvailable ? QStringLiteral("YES") : "NO"));
        results.append(QString("BuildManager error count: %1").arg(errorCount));
    } catch (...) {
        results.append("EXCEPTION: Error accessing BuildManager");
    }

    // Test 6: List all objects in PluginManager
    results.append("");
    results.append("Test 6: All PluginManager objects (first 20)");
    try {
        QObjectList allObjects = ExtensionSystem::PluginManager::allObjects();
        results.append(QString("Total objects in PluginManager: %1").arg(allObjects.size()));

        for (int i = 0; i < qMin(20, allObjects.size()); ++i) {
            QObject *obj = allObjects[i];
            if (obj) {
                QString className = QString::fromLatin1(obj->metaObject()->className());
                if (className.contains("Task", Qt::CaseInsensitive)
                    || className.contains("Issue", Qt::CaseInsensitive)
                    || className.contains("Output", Qt::CaseInsensitive)) {
                    results.append(QString("  [%1] %2 (%3)")
                                       .arg(i)
                                       .arg(className)
                                       .arg(reinterpret_cast<quintptr>(obj), 0, 16));
                }
            }
        }
    } catch (...) {
        results.append("EXCEPTION: Error listing PluginManager objects");
    }

    results.append("");
    results.append("=== END COMPREHENSIVE TASK ACCESS TEST ===");

    return results;
}

QString IssuesManager::formatTask(
    const QString &taskType,
    const QString &description,
    const QString &filePath,
    int lineNumber) const
{
    QString formatted = QString("%1:%2").arg(taskType, description);

    if (!filePath.isEmpty()) {
        formatted += QString(" [%1").arg(filePath);
        if (lineNumber > 0) {
            formatted += QString(":%1").arg(lineNumber);
        }
        formatted += "]";
    }

    return formatted;
}

} // namespace Mcp::Internal
