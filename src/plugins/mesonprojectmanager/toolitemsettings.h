// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QWidget>

#include <optional>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
class PathChooser;
}

namespace MesonProjectManager::Internal {

class ToolTreeItem;

class ToolItemSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ToolItemSettings(QWidget *parent = nullptr);
    void load(ToolTreeItem *item);
    void store();

signals:
    void applyChanges(Utils::Id itemId, const QString &name, const Utils::FilePath &exe);

private:
    std::optional<Utils::Id> m_currentId{std::nullopt};
    QLineEdit *m_mesonNameLineEdit;
    Utils::PathChooser *m_mesonPathChooser;
};

} // MesonProjectManager::Internal
