// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcukitaspect.h"
#include "mcusupporttr.h"

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

class McuDependenciesKitAspectImpl final : public KitAspect
{
public:
    McuDependenciesKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {}

    void makeReadOnly() override {}
    void refresh() override {}
    void addToLayoutImpl(Layouting::LayoutItem &) override {}
};

Utils::Id McuDependenciesKitAspect::id()
{
    return "PE.Profile.McuCMakeDependencies";
}

Utils::NameValueItems McuDependenciesKitAspect::dependencies(const Kit *kit)
{
    if (kit)
        return Utils::NameValueItem::fromStringList(
            kit->value(McuDependenciesKitAspect::id()).toStringList());
    return Utils::NameValueItems();
}

void McuDependenciesKitAspect::setDependencies(Kit *k, const Utils::NameValueItems &dependencies)
{
    if (k)
        k->setValue(McuDependenciesKitAspect::id(),
                    Utils::NameValueItem::toStringList(dependencies));
}

Utils::NameValuePairs McuDependenciesKitAspect::configuration(const Kit *kit)
{
    using namespace CMakeProjectManager;
    const auto config = CMakeConfigurationKitAspect::configuration(kit).toList();
    return Utils::transform<Utils::NameValuePairs>(config, [](const CMakeConfigItem &it) {
        return Utils::NameValuePair(QString::fromUtf8(it.key), QString::fromUtf8(it.value));
    });
}

//  McuDependenciesKitAspectFactory

class McuDependenciesKitAspectFactory final : public KitAspectFactory
{
public:
    McuDependenciesKitAspectFactory()
    {
        setId(McuDependenciesKitAspect::id());
        setDisplayName(Tr::tr("MCU Dependencies"));
        setDescription(Tr::tr("Paths to 3rd party dependencies"));
        setPriority(28500);
    }

    Tasks validate(const Kit *kit) const final
    {
        Tasks result;
        QTC_ASSERT(kit, return result);

        // check dependencies are defined properly for this kit
        const QVariant checkFormat = kit->value(McuDependenciesKitAspect::id());
        if (!checkFormat.isValid() || checkFormat.isNull())
            return result;
        if (!checkFormat.canConvert(QVariant::List))
            return {BuildSystemTask(Task::Error, Tr::tr("The MCU dependencies setting value is invalid."))};

        // check paths defined in cmake variables for given dependencies exist
        const auto cMakeEntries = Utils::NameValueDictionary(McuDependenciesKitAspect::configuration(kit));
        for (const auto &dependency : McuDependenciesKitAspect::dependencies(kit)) {
            auto givenPath = Utils::FilePath::fromUserInput(cMakeEntries.value(dependency.name));
            if (givenPath.isEmpty()) {
                result << BuildSystemTask(Task::Warning,
                                          Tr::tr("CMake variable %1 not defined.").arg(dependency.name));
            } else {
                const auto detectionPath = givenPath.resolvePath(dependency.value);
                if (!detectionPath.exists()) {
                    result << BuildSystemTask(Task::Warning,
                                              Tr::tr("CMake variable %1: path %2 does not exist.")
                                                  .arg(dependency.name, detectionPath.toUserOutput()));
                }
            }
        }

        return result;
    }
    void fix(Kit *kit) final
    {
        QTC_ASSERT(kit, return );

        const QVariant variant = kit->value(McuDependenciesKitAspect::id());
        if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
            qWarning("Kit \"%s\" has a wrong mcu dependencies value set.",
                     qPrintable(kit->displayName()));
            McuDependenciesKitAspect::setDependencies(kit, Utils::NameValueItems());
        }
    }

    KitAspect *createKitAspect(Kit *kit) const final
    {
        QTC_ASSERT(kit, return nullptr);
        return new McuDependenciesKitAspectImpl(kit, this);
    }

    ItemList toUserOutput(const Kit *) const final { return {}; }
};

const McuDependenciesKitAspectFactory theMcuDependenciesKitAspectFactory;

} // McuSupport::Internal
