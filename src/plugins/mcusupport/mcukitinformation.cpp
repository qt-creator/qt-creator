/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mcukitinformation.h"
#include "mcusupportcmakemapper.h"

#include <cmakeprojectmanager/cmakekitinformation.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>

using namespace ProjectExplorer;

namespace {

class McuDependenciesKitAspectWidget final : public KitAspectWidget
{
public:
    McuDependenciesKitAspectWidget(Kit *workingCopy, const KitAspect *ki)
        : KitAspectWidget(workingCopy, ki)
    {}

    void makeReadOnly() override {}
    void refresh() override {}
    void addToLayout(Utils::LayoutBuilder &) override {}
};

} // anonymous namespace

namespace McuSupport {
namespace Internal {

McuDependenciesKitAspect::McuDependenciesKitAspect()
{
    setObjectName(QLatin1String("McuDependenciesKitAspect"));
    setId(McuDependenciesKitAspect::id());
    setDisplayName(tr("MCU Dependencies"));
    setDescription(tr("Paths to 3rd party dependencies"));
    setPriority(28500);
}

Tasks McuDependenciesKitAspect::validate(const Kit *kit) const
{
    Tasks result;
    QTC_ASSERT(kit, return result);

    // check dependencies are defined properly for this kit
    const QVariant checkFormat = kit->value(McuDependenciesKitAspect::id());
    if (!checkFormat.isValid() || checkFormat.isNull())
        return result;
    if (!checkFormat.canConvert(QVariant::List))
        return {BuildSystemTask(Task::Error, tr("The MCU dependencies setting value is invalid."))};

    // check paths defined in cmake variables for given dependencies exist
    const auto cMakeEntries = Utils::NameValueDictionary(configuration(kit));
    for (const auto &dependency: dependencies(kit)) {
        auto givenPath = Utils::FilePath::fromString(cMakeEntries.value(dependency.name));
        if (givenPath.isEmpty()) {
            result << BuildSystemTask(Task::Warning, tr("CMake variable %1 not defined.").arg(
                                          dependency.name));
        } else {
            const auto detectionPath = givenPath.resolvePath(dependency.value);
            if (!detectionPath.exists()) {
                result << BuildSystemTask(Task::Warning, tr("CMake variable %1: path %2 does not exist.").arg(
                                              dependency.name,
                                              detectionPath.toUserOutput()));
            }
        }
    }

    return result;
}

void McuDependenciesKitAspect::fix(Kit *kit)
{
    QTC_ASSERT(kit, return);

    const QVariant variant = kit->value(McuDependenciesKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong mcu dependencies value set.", qPrintable(kit->displayName()));
        setDependencies(kit, Utils::NameValueItems());
    }
}

KitAspectWidget *McuDependenciesKitAspect::createConfigWidget(Kit *kit) const
{
    QTC_ASSERT(kit, return nullptr);
    return new McuDependenciesKitAspectWidget(kit, this);
}

KitAspect::ItemList McuDependenciesKitAspect::toUserOutput(const Kit *kit) const
{
    Q_UNUSED(kit)

    return {};
}

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
    return Utils::transform(config, [](const CMakeConfigItem &it) {
        return Utils::NameValuePair(QString::fromUtf8(it.key), QString::fromUtf8(it.value));
    });
}

} // namespace Internal
} // namespace McuSupport
