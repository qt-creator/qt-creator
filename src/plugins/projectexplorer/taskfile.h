// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itaskhandler.h"

#include <coreplugin/idocument.h>

namespace ProjectExplorer {
namespace Internal {

class StopMonitoringHandler : public ITaskHandler
{
public:
    bool canHandle(const ProjectExplorer::Task &) const override;
    void handle(const ProjectExplorer::Task &) override;
    QAction *createAction(QObject *parent) const override;
};

class TaskFile : public Core::IDocument
{
public:
    TaskFile(QObject *parent);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    bool load(QString *errorString, const Utils::FilePath &fileName);

    static TaskFile *openTasks(const Utils::FilePath &filePath);
    static void stopMonitoring();

private:
    static QList<TaskFile *> openFiles;
};

} // namespace Internal
} // namespace ProjectExplorer
