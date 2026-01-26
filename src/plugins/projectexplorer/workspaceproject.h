// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "project.h"

#include "treescanner.h"

#include <utils/filepath.h>
#include <utils/filesystemwatcher.h>
#include <utils/store.h>

#include <QJsonObject>
#include <qglobal.h>

namespace Core {
class IVersionControl;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT WorkspaceProject : public Project
{
    Q_OBJECT
public:
    WorkspaceProject(const Utils::FilePath &file, const QJsonObject &defaultConfiguration = {});

    Utils::FilePath projectDirectory() const override;
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;

    void excludeNode(Node *node);

    void setFilters(const QList<QRegularExpression> &filters) { m_filters = filters; }
    const QList<QRegularExpression> &filters() const { return m_filters; }
    void scan(const Utils::FilePath &path);

private:
    void updateBuildConfigurations();
    void saveProjectDefinition(const QJsonObject &json);
    void excludePath(const Utils::FilePath &path);
    bool isFiltered(
        const Utils::FilePath &path, QList<Core::IVersionControl *> versionControls) const;
    void handleDirectoryChanged(const Utils::FilePath &directory);
    void setupScanner();
    void scanNext();

private:
    Utils::FilePaths m_scanQueue;
    TreeScanner m_scanner;
    QList<QRegularExpression> m_filters;
    std::unique_ptr<Utils::FileSystemWatcher> m_watcher;
};

void setupWorkspaceProject(QObject *guard);

} // namespace ProjectExplorer
