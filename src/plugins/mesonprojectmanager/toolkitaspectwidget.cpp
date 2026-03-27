// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolkitaspectwidget.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesontools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <utils/qtcassert.h>

#include <QAbstractListModel>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

class MesonListModel final : public QAbstractListModel
{
public:
    using QAbstractListModel::QAbstractListModel;

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = {}) const final
    {
        return parent.isValid() ? 0 : int(MesonTools::tools().size());
    }

    QVariant data(const QModelIndex &index, int role) const final
    {
        const std::vector<MesonTools::Tool_t> &tools = MesonTools::tools();
        if (!index.isValid() || index.row() < 0 || index.row() >= int(tools.size()))
            return {};
        const MesonTools::Tool_t &tool = tools[index.row()];
        switch (role) {
        case Qt::DisplayRole:
            return tool->name();
        case KitAspect::IdRole:
            return tool->id().toSetting();
        }
        return {};
    }
};

// MesonToolKitAspectImpl

class MesonToolKitAspectImpl final : public KitAspect
{
public:
    MesonToolKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory)
    {
        setManagingPage(Constants::SettingsPage::TOOLS_ID);

        auto getter = [](const Kit &k) { return MesonToolKitAspect::mesonToolId(&k).toSetting(); };
        auto setter = [](Kit &k, const QVariant &v) {
            MesonToolKitAspect::setMesonTool(&k, Id::fromSetting(v));
        };
        auto reset = [this] { model.reset(); };
        addListAspectSpec({&model, getter, setter, reset});

        connect(MesonTools::instance(), &MesonTools::toolsChanged, this, &KitAspect::refresh);
    }

    MesonListModel model;
};

// MesonToolKitAspect

const char MESON_TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Meson";

void MesonToolKitAspect::setMesonTool(Kit *kit, Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(MESON_TOOL_ID, id.toSetting());
}

Id MesonToolKitAspect::mesonToolId(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Id::fromSetting(kit->value(MESON_TOOL_ID));
}

std::shared_ptr<MesonToolWrapper> MesonToolKitAspect::mesonTool(const Kit *kit)
{
    return MesonTools::toolById(MesonToolKitAspect::mesonToolId(kit));
}

bool MesonToolKitAspect::isValid(const Kit *kit)
{
    auto tool = mesonTool(kit);
    return tool && tool->isValid();
}

// MesonToolKitAspectFactory

class MesonToolKitAspectFactory final : public KitAspectFactory
{
public:
    MesonToolKitAspectFactory()
    {
        setId(MESON_TOOL_ID);
        setDisplayName(Tr::tr("Meson Tool"));
        setDescription(Tr::tr("The Meson tool to use when building a project with Meson.<br>"
                              "This setting is ignored when using other build systems."));
        setPriority(9000);
    }

    Tasks validate(const Kit *k) const final
    {
        Tasks tasks;
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool && !tool->isValid())
            tasks << BuildSystemTask{Task::Warning, Tr::tr("Cannot validate this meson executable.")};
        return tasks;
    }

    void setup(Kit *k) final
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (!tool) {
            const auto autoDetected = MesonTools::autoDetectedTool();
            if (autoDetected)
                MesonToolKitAspect::setMesonTool(k, autoDetected->id());
        }
    }

    void fix(Kit *k) final
    {
        setup(k);
    }

    KitAspect *createKitAspect(Kit *k) const final
    {
        return new MesonToolKitAspectImpl(k, this);
    }

    ItemList toUserOutput(const Kit *k) const final
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool)
            return {{Tr::tr("Meson"), tool->name()}};
        return {{Tr::tr("Meson"), Tr::tr("Unconfigured")}};
    }
};

const MesonToolKitAspectFactory theMesonKitAspectFactory;

} // MesonProjectManager::Internal
