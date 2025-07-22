// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/treemodel.h>

#include <projectexplorer/kitaspect.h>

namespace CMakeProjectManager {
class CMakeTool;

namespace Internal {

void setupCMakeSettingsPage();

class CMakeToolTreeItem : public Utils::TreeItem
{
public:
    CMakeToolTreeItem() = default;
    CMakeToolTreeItem(const CMakeTool *item, bool changed);
    CMakeToolTreeItem(
        const QString &name,
        const Utils::FilePath &executable,
        const Utils::FilePath &qchFile,
        bool autoRun,
        const ProjectExplorer::DetectionSource &detectionSource);

    void updateErrorFlags();
    bool hasError() const;

    static const int DefaultItemIdRole = Qt::UserRole + 1;
    QVariant data(int column, int role) const override;

    Utils::Id m_id;
    QString m_name;
    QString m_tooltip;
    Utils::FilePath m_executable;
    Utils::FilePath m_qchFile;
    QString m_versionDisplay;
    ProjectExplorer::DetectionSource m_detectionSource;
    bool m_isAutoRun = true;
    bool m_pathExists = false;
    bool m_pathIsFile = false;
    bool m_pathIsExecutable = false;
    bool m_isSupported = false;
    bool m_changed = true;
};

} // Internal
} // CMakeProjectManager
