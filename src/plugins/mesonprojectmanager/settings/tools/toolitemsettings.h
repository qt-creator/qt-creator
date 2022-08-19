// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/fileutils.h>
#include <utils/optional.h>

#include <QWidget>

namespace MesonProjectManager {
namespace Internal {

namespace Ui { class ToolItemSettings; }

class ToolTreeItem;

class ToolItemSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ToolItemSettings(QWidget *parent = nullptr);
    ~ToolItemSettings();
    void load(ToolTreeItem *item);
    void store();
    Q_SIGNAL void applyChanges(Utils::Id itemId, const QString &name, const Utils::FilePath &exe);

private:
    Ui::ToolItemSettings *ui;
    Utils::optional<Utils::Id> m_currentId{Utils::nullopt};
};

} // namespace Internal
} // namespace MesonProjectManager
