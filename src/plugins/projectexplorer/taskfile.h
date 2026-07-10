// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "task.h"
#include "taskhandlers.h"

#include <coreplugin/idocument.h>

#include <optional>

namespace ProjectExplorer::Internal {

// Parses one raw line of a .tasks file. Returns nullopt for comment and blank
// lines; a returned task may still have an empty description, which the file
// loader treats as invalid.
std::optional<Task> parseTaskFileLine(const QByteArray &rawLine,
                                      const Utils::FilePath &parentDir);

class StopMonitoringHandler : public ITaskHandler
{
public:
    StopMonitoringHandler() : ITaskHandler(createAction()) {}

private:
    bool canHandle(const ProjectExplorer::Task &) const override;
    void handle(const ProjectExplorer::Task &) override;
    QAction *createAction() const;
};

class TaskFile : public Core::IDocument
{
public:
    TaskFile(QObject *parent);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    Utils::Result<> reload(ReloadFlag flag, ChangeType type) override;

    bool load(QString *errorString, const Utils::FilePath &fileName);

    static TaskFile *openTasks(const Utils::FilePath &filePath);
    static void stopMonitoring();

private:
    static QList<TaskFile *> openFiles;
};

} // namespace ProjectExplorer::Internal
