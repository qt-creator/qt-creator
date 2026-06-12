// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettings.h"

using namespace Utils;

namespace ProjectExplorer {

Key useGlobalSettingsKey()
{
    return "useGlobalSettings";
}

void setupUseGlobalSettings(AspectContainer *container, UseGlobalAspect *useGlobal,
                            const std::function<void()> &save)
{
    container->setEnabled(!useGlobal->value());
    useGlobal->addOnChanged(container, [container, useGlobal, save] {
        container->setEnabled(!useGlobal->value());
        save();
    });
    container->addOnChanged(container, [useGlobal, save] {
        if (!useGlobal->value())
            save();
    });
}

void loadProjectSettings(AspectContainer &container, UseGlobalAspect &useGlobal,
                         Project *project, const Key &settingsKey)
{
    const Store data = storeFromVariant(project->namedSettings(settingsKey));
    container.fromMap(data);
    useGlobal.setValue(data.value(useGlobalSettingsKey(), true).toBool());
}

void saveProjectSettings(const AspectContainer &container, const UseGlobalAspect &useGlobal,
                         Project *project, const Key &settingsKey)
{
    if (useGlobal.value() && project->namedSettings(settingsKey).isNull())
        return;
    Store data;
    container.toMap(data);
    data.insert(useGlobalSettingsKey(), useGlobal.value());
    project->setNamedSettings(settingsKey, variantFromStore(data));
}

} // namespace ProjectExplorer
