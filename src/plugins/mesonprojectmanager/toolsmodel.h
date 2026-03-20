// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <utils/groupedmodel.h>

namespace MesonProjectManager::Internal {

struct ToolItem
{
    ToolItem() = default;
    explicit ToolItem(const QString &name);
    explicit ToolItem(const MesonTools::Tool_t &tool);
    ToolItem cloned() const;

    QVariant data(int column, int role) const;
    void update(const QString &name, const Utils::FilePath &exe);

    friend bool operator==(const ToolItem &, const ToolItem &) = default;

    QString name;
    QString tooltip;
    Utils::FilePath executable;
    Utils::Id id;
    bool autoDetected = false;
    bool pathExists = false;
    bool pathIsFile = false;
    bool pathIsExecutable = false;

private:
    void selfCheck();
    void updateTooltip();
    void updateTooltip(const QVersionNumber &version);
};

class ToolsModel final : public Utils::TypedGroupedModel<ToolItem>
{
public:
    ToolsModel();

    int addMesonTool();
    int cloneMesonTool(int row);
    void updateItem(const Utils::Id &itemId, const QString &name, const Utils::FilePath &exe);
    int rowForId(const Utils::Id &id) const;
    void apply() override;

private:
    QVariant variantData(const QVariant &v, int column, int role) const override;
    QString uniqueName(const QString &baseName) const;
};

} // namespace MesonProjectManager::Internal

Q_DECLARE_METATYPE(MesonProjectManager::Internal::ToolItem)
