// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QString>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

class ToolTreeItem final : public Utils::TreeItem
{
public:
    ToolTreeItem(const QString &name);
    ToolTreeItem(const MesonTools::Tool_t &tool);
    ToolTreeItem(const ToolTreeItem &other);

    QVariant data(int column, int role) const override;
    inline bool isAutoDetected() const noexcept { return m_autoDetected; }
    inline QString name() const noexcept { return m_name; }
    inline Utils::FilePath executable() const noexcept { return m_executable; }
    inline Utils::Id id() const noexcept { return m_id; }
    inline bool hasUnsavedChanges() const noexcept { return m_unsavedChanges; }
    inline void setSaved() { m_unsavedChanges = false; }
    void update(const QString &name, const Utils::FilePath &exe);

private:
    void self_check();
    void update_tooltip(const Version &version);
    void update_tooltip();
    QString m_name;
    QString m_tooltip;
    Utils::FilePath m_executable;
    bool m_autoDetected;
    bool m_pathExists;
    bool m_pathIsFile;
    bool m_pathIsExecutable;
    Utils::Id m_id;

    bool m_unsavedChanges = false;
};

} // namespace Internal
} // namespace MesonProjectManager
