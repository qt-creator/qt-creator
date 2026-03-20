// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnkitaspect.h"

#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>

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

// GNKitAspectImpl (widget)

class GNKitAspectImpl final : public KitAspect
{
public:
    GNKitAspectImpl(Kit *kit, const KitAspectFactory *factory);
    ~GNKitAspectImpl() override { delete m_toolsComboBox; }

private:
    void addTool(const GNTools::Tool_t &tool);
    void removeTool(const GNTools::Tool_t &tool);
    void setCurrentToolIndex(int index);
    int indexOf(const Id &id);
    void loadTools();
    void setToDefault();

    void makeReadOnly(bool readOnly) final { m_toolsComboBox->setEnabled(!readOnly); }

    void addToInnerLayout(Layouting::Layout &layout) final
    {
        addMutableAction(m_toolsComboBox);
        layout.addItem(m_toolsComboBox);
    }

    void refresh() final
    {
        const auto id = GNKitAspect::gnToolId(kit());
        m_toolsComboBox->setCurrentIndex(indexOf(id));
    }

    QComboBox *m_toolsComboBox;
};

GNKitAspectImpl::GNKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
    : KitAspect(kit, factory)
    , m_toolsComboBox(createSubWidget<QComboBox>())
{
    setManagingPage(Constants::SettingsPage::TOOLS_ID);

    m_toolsComboBox->setSizePolicy(QSizePolicy::Ignored,
                                   m_toolsComboBox->sizePolicy().verticalPolicy());
    m_toolsComboBox->setEnabled(false);
    m_toolsComboBox->setToolTip(factory->description());
    loadTools();

    connect(GNTools::instance(), &GNTools::toolAdded, this, &GNKitAspectImpl::addTool);
    connect(GNTools::instance(), &GNTools::toolRemoved, this, &GNKitAspectImpl::removeTool);
    connect(m_toolsComboBox,
            &QComboBox::currentIndexChanged,
            this,
            &GNKitAspectImpl::setCurrentToolIndex);
}

void GNKitAspectImpl::addTool(const GNTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return);
    m_toolsComboBox->addItem(tool->name(), tool->id().toSetting());
}

void GNKitAspectImpl::removeTool(const GNTools::Tool_t &tool)
{
    QTC_ASSERT(tool, return);
    const int index = indexOf(tool->id());
    QTC_ASSERT(index >= 0, return);
    if (index == m_toolsComboBox->currentIndex())
        setToDefault();
    m_toolsComboBox->removeItem(index);
}

void GNKitAspectImpl::setCurrentToolIndex(int index)
{
    if (m_toolsComboBox->count() == 0)
        return;
    const Id id = Id::fromSetting(m_toolsComboBox->itemData(index));
    GNKitAspect::setGNTool(kit(), id);
}

int GNKitAspectImpl::indexOf(const Id &id)
{
    for (int i = 0; i < m_toolsComboBox->count(); ++i) {
        if (id == Id::fromSetting(m_toolsComboBox->itemData(i)))
            return i;
    }
    return -1;
}

void GNKitAspectImpl::loadTools()
{
    for (const GNTools::Tool_t &tool : GNTools::tools())
        addTool(tool);
    refresh();
    m_toolsComboBox->setEnabled(m_toolsComboBox->count());
}

void GNKitAspectImpl::setToDefault()
{
    const GNTools::Tool_t autoDetected = GNTools::autoDetectedTool();
    if (autoDetected) {
        const auto index = indexOf(autoDetected->id());
        m_toolsComboBox->setCurrentIndex(index);
        setCurrentToolIndex(index);
    } else {
        m_toolsComboBox->setCurrentIndex(0);
        setCurrentToolIndex(0);
    }
}

// GNKitAspectFactory

class GNKitAspectFactory final : public KitAspectFactory
{
public:
    GNKitAspectFactory()
    {
        setId(GN_TOOL_ID);
        setDisplayName(Tr::tr("GN Tool"));
        setDescription(Tr::tr("The GN tool to use when building a project with GN.<br>"
                              "This setting is ignored when using other build systems."));
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
        const auto tool = GNKitAspect::gnTool(k);
        if (!tool) {
            const auto autoDetected = GNTools::autoDetectedTool();
            if (autoDetected)
                GNKitAspect::setGNTool(k, autoDetected->id());
        }
    }

    void fix(Kit *k) final { setup(k); }

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
