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

#include <utils/qtcassert.h>

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

Tasks McuDependenciesKitAspect::validate(const Kit *k) const
{
    Tasks result;
    QTC_ASSERT(k, return result);

    const QVariant checkFormat = k->value(McuDependenciesKitAspect::id());
    if (!checkFormat.isNull() && !checkFormat.canConvert(QVariant::List))
        return { BuildSystemTask(Task::Error, tr("The MCU dependencies setting value is invalid.")) };

    const QVariant envStringList = k->value(EnvironmentKitAspect::id());
    if (!envStringList.isNull() && !envStringList.canConvert(QVariant::List))
         return { BuildSystemTask(Task::Error, tr("The environment setting value is invalid.")) };

    const auto environment = Utils::NameValueDictionary(envStringList.toStringList());
    for (const auto &dependency: dependencies(k)) {
        if (!environment.hasKey(dependency.name)) {
            result << BuildSystemTask(Task::Warning, tr("Environment variable %1 not defined.").arg(dependency.name));
        } else {
            const auto path = Utils::FilePath::fromUserInput(
                        environment.value(dependency.name) + "/" + dependency.value);
            if (!path.exists()) {
                result << BuildSystemTask(Task::Warning, tr("%1 not found.").arg(path.toUserOutput()));
            }
        }
    }

    return result;
}

void McuDependenciesKitAspect::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant variant = k->value(McuDependenciesKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong mcu dependencies value set.", qPrintable(k->displayName()));
        setDependencies(k, Utils::NameValueItems());
    }
}

KitAspectWidget *McuDependenciesKitAspect::createConfigWidget(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new McuDependenciesKitAspectWidget(k, this);
}

KitAspect::ItemList McuDependenciesKitAspect::toUserOutput(const Kit *k) const
{
    Q_UNUSED(k)

    return {};
}

Utils::Id McuDependenciesKitAspect::id()
{
    return "PE.Profile.McuDependencies";
}


Utils::NameValueItems McuDependenciesKitAspect::dependencies(const Kit *k)
{
     if (k)
         return Utils::NameValueItem::fromStringList(k->value(McuDependenciesKitAspect::id()).toStringList());
     return Utils::NameValueItems();
}

void McuDependenciesKitAspect::setDependencies(Kit *k, const Utils::NameValueItems &dependencies)
{
    if (k)
        k->setValue(McuDependenciesKitAspect::id(), Utils::NameValueItem::toStringList(dependencies));
}

} // namespace Internal
} // namespace McuSupport
