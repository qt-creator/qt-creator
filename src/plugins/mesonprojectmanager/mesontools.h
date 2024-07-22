// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "toolwrapper.h"

#include <utils/algorithm.h>

#include <memory>

namespace MesonProjectManager {
namespace Internal {

class MesonTools : public QObject
{
    Q_OBJECT
    MesonTools() {}
    ~MesonTools() {}

public:
    using Tool_t = std::shared_ptr<ToolWrapper>;

    static bool isMesonWrapper(const Tool_t &tool);
    static bool isNinjaWrapper(const Tool_t &tool);

    static void addTool(const Utils::Id &itemId,
                        const QString &name,
                        const Utils::FilePath &exe);

    static void addTool(Tool_t meson);

    static void setTools(std::vector<Tool_t> &&tools);

    static inline const std::vector<Tool_t> &tools() { return instance()->m_tools; }

    static void updateTool(const Utils::Id &itemId,
                           const QString &name,
                           const Utils::FilePath &exe);
    static void removeTool(const Utils::Id &id);

    static std::shared_ptr<ToolWrapper> ninjaWrapper(const Utils::Id &id);
    static std::shared_ptr<ToolWrapper> mesonWrapper(const Utils::Id &id);

    static std::shared_ptr<ToolWrapper> autoDetectedNinja();
    static std::shared_ptr<ToolWrapper> autoDetectedMeson();

    Q_SIGNAL void toolAdded(const Tool_t &tool);
    Q_SIGNAL void toolRemoved(const Tool_t &tool);

    static MesonTools *instance()
    {
        static MesonTools inst;
        return &inst;
    }

private:
    std::vector<Tool_t> m_tools;
};

} // namespace Internal
} // namespace MesonProjectManager
