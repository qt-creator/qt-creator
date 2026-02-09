// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

// Include for MOC compilation
#include <projectexplorer/task.h>

namespace Mcp::Internal {

/**
 * @brief Manages access to Qt Creator's Issues panel
 *]
 * This class provides a clean interface for accessing and retrieving
 * issues from Qt Creator's Issues panel. It encapsulates the complexity
 * of accessing internal Qt Creator APIs and provides a simple interface
 * for the MCP plugin.
 */
class IssuesManager : public QObject
{
    Q_OBJECT

public:
    explicit IssuesManager(QObject *parent = nullptr);
    ~IssuesManager() override = default;

    /**
     * @brief Retrieves all current issues from the Issues panel
     * @return JSON object conforming to issues-schema.json
     */
    QJsonObject getCurrentIssues() const;

    /**
     * @brief Retrieves the issues schema from resources
     * @return JSON object containing the schema, or empty object on error
     */
    static QJsonObject issuesSchema();

    /**
     * @brief Tests multiple approaches to access Qt Creator's task data
     * @return List of test results and findings
     */
    QStringList testTaskAccess() const;

private slots:
    /**
     * @brief Handles task added signals from TaskHub
     * @param task The task that was added
     */
    void onTaskAdded(const ProjectExplorer::Task &task);

    /**
     * @brief Handles task removed signals from TaskHub
     * @param task The task that was removed
     */
    void onTaskRemoved(const ProjectExplorer::Task &task);

    /**
     * @brief Handles tasks changed signal from TaskWindow
     */
    void onTasksChanged();

    /**
     * @brief Checks if the Issues panel is accessible
     * @return true if accessible, false otherwise
     */
    bool isAccessible() const;

    /**
     * @brief Gets the count of current issues
     * @return Number of issues, or -1 if not accessible
     */
    int getIssueCount() const;

private:
    /**
     * @brief Attempts to access the Issues panel through various methods
     * @return true if successful, false otherwise
     */
    bool initializeAccess();

    /**
     * @brief Formats a task into a readable string
     * @param taskType The type of task (Error, Warning, etc.)
     * @param description The task description
     * @param filePath The file path (if available)
     * @param lineNumber The line number (if available)
     * @return Formatted string
     */
    QString formatTask(
        const QString &taskType,
        const QString &description,
        const QString &filePath = QString(),
        int lineNumber = -1) const;

    /**
     * @brief Connects to TaskHub and TaskWindow signals
     */
    void connectSignals();

    bool m_accessible = false;

    // Task tracking
    QList<ProjectExplorer::Task> m_trackedTasks;
    QObject *m_taskWindow = nullptr;
    bool m_signalsConnected = false;
};

} // namespace Mcp::Internal
