// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "project.h"

#include <utils/store.h>

#include <QJsonObject>
#include <qglobal.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT WorkspaceProject : public Project
{
    Q_OBJECT
public:
    WorkspaceProject(const Utils::FilePath &file, const QJsonObject &defaultConfiguration = {});

    Utils::FilePath projectDirectory() const override;
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;

    void excludeNode(Node *node);

private:
    void updateBuildConfigurations();
    void saveProjectDefinition(const QJsonObject &json);
    void excludePath(const Utils::FilePath &path);
};

void setupWorkspaceProject(QObject *guard);

} // namespace ProjectExplorer
