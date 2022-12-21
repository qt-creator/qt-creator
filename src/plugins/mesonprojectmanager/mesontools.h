// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
#include "mesonwrapper.h"
#include "ninjawrapper.h"
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

    static inline void addTool(const Utils::Id &itemId,
                               const QString &name,
                               const Utils::FilePath &exe)
    {
        // TODO improve this
        if (exe.fileName().contains("ninja"))
            addTool(std::make_shared<NinjaWrapper>(name, exe, itemId));
        else
            addTool(std::make_shared<MesonWrapper>(name, exe, itemId));
    }

    static inline void addTool(Tool_t meson)
    {
        auto self = instance();
        self->m_tools.emplace_back(std::move(meson));
        emit self->toolAdded(self->m_tools.back());
    }

    static void setTools(std::vector<Tool_t> &&tools);

    static inline const std::vector<Tool_t> &tools() { return instance()->m_tools; }

    static inline void updateTool(const Utils::Id &itemId,
                                  const QString &name,
                                  const Utils::FilePath &exe)
    {
        auto self = instance();
        auto item = std::find_if(std::begin(self->m_tools),
                                 std::end(self->m_tools),
                                 [&itemId](const Tool_t &tool) { return tool->id() == itemId; });
        if (item != std::end(self->m_tools)) {
            (*item)->setExe(exe);
            (*item)->setName(name);
        } else {
            addTool(itemId, name, exe);
        }
    }
    static void removeTool(const Utils::Id &id)
    {
        auto self = instance();
        auto item = Utils::take(self->m_tools, [&id](const auto &item) { return item->id() == id; });
        QTC_ASSERT(item, return );
        emit self->toolRemoved(*item);
    }

    static std::shared_ptr<NinjaWrapper> ninjaWrapper(const Utils::Id &id);
    static std::shared_ptr<MesonWrapper> mesonWrapper(const Utils::Id &id);

    static std::shared_ptr<NinjaWrapper> ninjaWrapper();
    static std::shared_ptr<MesonWrapper> mesonWrapper();

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
