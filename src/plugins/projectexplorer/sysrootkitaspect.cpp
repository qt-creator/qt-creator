// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sysrootkitaspect.h"

#include "kit.h"
#include "kitaspect.h"
#include "projectexplorertr.h"
#include "toolchain.h"
#include "toolchainkitaspect.h"

#include <utils/filepath.h>
#include <utils/guard.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {
class SysRootKitAspectImpl : public KitAspect
{
public:
    SysRootKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        m_chooser = createSubWidget<PathChooser>();
        m_chooser->setExpectedKind(PathChooser::ExistingDirectory);
        m_chooser->setHistoryCompleter("PE.SysRoot.History");
        m_chooser->setFilePath(SysRootKitAspect::sysRoot(k));
        connect(m_chooser, &PathChooser::textChanged,
                this, &SysRootKitAspectImpl::pathWasChanged);
    }

    ~SysRootKitAspectImpl() override { delete m_chooser; }

private:
    void makeReadOnly() override { m_chooser->setReadOnly(true); }

    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_chooser);
        layout.addItem(Layouting::Span(2, m_chooser));
    }

    void refresh() override
    {
        if (!m_ignoreChanges.isLocked())
            m_chooser->setFilePath(SysRootKitAspect::sysRoot(kit()));
    }

    void pathWasChanged()
    {
        const GuardLocker locker(m_ignoreChanges);
        SysRootKitAspect::setSysRoot(kit(), m_chooser->filePath());
    }

    PathChooser *m_chooser;
    Guard m_ignoreChanges;
};

class SysRootKitAspectFactory : public KitAspectFactory
{
public:
    SysRootKitAspectFactory();

    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;
};

SysRootKitAspectFactory::SysRootKitAspectFactory()
{
    setId(SysRootKitAspect::id());
    setDisplayName(Tr::tr("Sysroot"));
    setDescription(Tr::tr("The root directory of the system image to use.<br>"
                          "Leave empty when building for the desktop."));
    setPriority(27000);
}

Tasks SysRootKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    const FilePath dir = SysRootKitAspect::sysRoot(k);
    if (dir.isEmpty())
        return result;

    if (dir.startsWith("target:") || dir.startsWith("remote:"))
        return result;

    if (!dir.exists()) {
        result << BuildSystemTask(Task::Warning,
                                  Tr::tr("Sys Root \"%1\" does not exist in the file system.").arg(dir.toUserOutput()));
    } else if (!dir.isDir()) {
        result << BuildSystemTask(Task::Warning,
                                  Tr::tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()));
    } else if (dir.dirEntries(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        result << BuildSystemTask(Task::Warning,
                                  Tr::tr("Sys Root \"%1\" is empty.").arg(dir.toUserOutput()));
    }
    return result;
}

KitAspect *SysRootKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);

    return new Internal::SysRootKitAspectImpl(k, this);
}

KitAspectFactory::ItemList SysRootKitAspectFactory::toUserOutput(const Kit *k) const
{
    return {{Tr::tr("Sys Root"), SysRootKitAspect::sysRoot(k).toUserOutput()}};
}

void SysRootKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);

    expander->registerFileVariables("SysRoot", Tr::tr("Sys Root"), [kit] {
        return SysRootKitAspect::sysRoot(kit);
    });
}

const SysRootKitAspectFactory theSyRootKitAspectFactory;

} // namespace Internal

Id SysRootKitAspect::id()
{
    return "PE.Profile.SysRoot";
}

FilePath SysRootKitAspect::sysRoot(const Kit *k)
{
    if (!k)
        return {};

    if (!k->value(SysRootKitAspect::id()).toString().isEmpty())
        return FilePath::fromSettings(k->value(SysRootKitAspect::id()));

    for (Toolchain *tc : ToolchainKitAspect::toolChains(k)) {
        if (!tc->sysRoot().isEmpty())
            return FilePath::fromString(tc->sysRoot());
    }
    return {};
}

void SysRootKitAspect::setSysRoot(Kit *k, const FilePath &v)
{
    if (!k)
        return;

    for (Toolchain *tc : ToolchainKitAspect::toolChains(k)) {
        if (!tc->sysRoot().isEmpty()) {
            // It's the sysroot from toolchain, don't set it.
            if (tc->sysRoot() == v.toString())
                return;

            // We've changed the default toolchain sysroot, set it.
            break;
        }
    }
    k->setValue(SysRootKitAspect::id(), v.toString());
}

} // namespace ProjectExplorer
