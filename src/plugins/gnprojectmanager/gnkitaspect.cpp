// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnkitaspect.h"

#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <utils/qtcassert.h>

#include <QAbstractListModel>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

// GNKitAspect

const char GN_TOOL_ID[] = "GNProjectManager.GNKitInformation.GN";

void GNKitAspect::setGNTool(Kit *kit, Id id)
{
    QTC_ASSERT(kit && id.isValid(), return);
    kit->setValue(GN_TOOL_ID, id.toSetting());
}

Id GNKitAspect::gnToolId(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Id::fromSetting(kit->value(GN_TOOL_ID));
}

std::shared_ptr<GNTool> GNKitAspect::gnTool(const Kit *kit)
{
    return GNTools::toolById(GNKitAspect::gnToolId(kit));
}

bool GNKitAspect::isValid(const Kit *kit)
{
    auto tool = gnTool(kit);
    return tool && tool->isValid();
}

// GNToolListModel

class GNToolListModel final : public QAbstractListModel
{
public:
    void reset()
    {
        beginResetModel();
        endResetModel();
    }

    int rowCount(const QModelIndex &parent) const final
    {
        return parent.isValid() ? 0 : GNTools::toolCount() + 1;
    }

    QVariant data(const QModelIndex &index, int role) const final
    {
        if (!index.isValid() || index.row() < 0 || index.row() > GNTools::toolCount())
            return {};
        return GNTools::data(index.row() - 1, role);
    }
};

// GNKitAspectImpl (widget)

class GNKitAspectImpl final : public KitAspect
{
public:
    GNKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory)
    {
        setManagingPage(Constants::SettingsPage::TOOLS_ID);

        auto getter = [](const Kit &k) -> QVariant {
            const Id id = GNKitAspect::gnToolId(&k);
            return id.isValid() ? id.toSetting() : QVariant{};
        };
        auto setter = [](Kit &k, const QVariant &v) { k.setValue(GN_TOOL_ID, v); };
        auto resetModel = [this] { m_model.reset(); };
        addListAspectSpec({&m_model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(GNTools::instance(), &GNTools::toolsChanged, this, &KitAspect::refresh);
    }

    GNToolListModel m_model;
};

// GNKitAspectFactory

class GNKitAspectFactory final : public KitAspectFactory
{
public:
    GNKitAspectFactory()
    {
        setId(GN_TOOL_ID);
        setDisplayName(Tr::tr("GN Tool"));
        setDescription(
            "<qt>"
            + Tr::tr(
                "The GN tool to use when building a project with GN. This setting is ignored when "
                "using other build systems."));
        setPriority(9000);
    }

    Tasks validate(const Kit *k) const final
    {
        Tasks tasks;
        const auto tool = GNKitAspect::gnTool(k);
        if (tool && !tool->isValid())
            tasks << BuildSystemTask{Task::Warning, Tr::tr("Cannot validate this GN executable.")};
        return tasks;
    }

    void setup(Kit *k) final
    {
        if (k->hasValue(GN_TOOL_ID))
            return;
        const auto autoDetected = GNTools::autoDetectedTool();
        if (autoDetected)
            GNKitAspect::setGNTool(k, autoDetected->id());
    }

    void fix(Kit *k) final
    {
        const Id id = GNKitAspect::gnToolId(k);
        if (!id.isValid())
            return;
        if (GNTools::toolById(id))
            return;
        const auto autoDetected = GNTools::autoDetectedTool();
        if (autoDetected)
            GNKitAspect::setGNTool(k, autoDetected->id());
        else
            k->setValue(GN_TOOL_ID, QVariant{});
    }

    KitAspect *createKitAspect(Kit *k) const final { return new GNKitAspectImpl(k, this); }

    ItemList toUserOutput(const Kit *k) const final
    {
        const auto tool = GNKitAspect::gnTool(k);
        if (tool)
            return {{Tr::tr("GN"), tool->name()}};
        return {{Tr::tr("GN"), Tr::tr("Unconfigured")}};
    }
};

void setupGNKitAspect()
{
    static GNKitAspectFactory theGNKitAspectFactory;
}

} // namespace GNProjectManager::Internal
