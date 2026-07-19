// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "project.h"
#include "useglobalaspect.h"

#include <utils/aspects.h>

#include <functional>

namespace ProjectExplorer {

// Key under which the "use global settings" flag is stored inside a project's
// settings store. Shared by all project settings built around UseGlobalAspect.
PROJECTEXPLORER_EXPORT Utils::Key useGlobalSettingsKey();

// Wires up the common "use global settings" behavior on \a container: disables
// it while the global settings are in effect, and calls \a save whenever the
// flag changes, or whenever a project setting changes while not using the
// global settings. \a save is expected to both persist and apply the settings.
PROJECTEXPLORER_EXPORT void setupUseGlobalSettings(Utils::AspectContainer *container,
                                                   UseGlobalAspect *useGlobal,
                                                   const std::function<void()> &save);

// Loads the per-project store named \a settingsKey into \a container and sets
// \a useGlobal from the stored flag (defaulting to true for fresh projects).
PROJECTEXPLORER_EXPORT void loadProjectSettings(Utils::AspectContainer &container,
                                                UseGlobalAspect &useGlobal,
                                                Project *project,
                                                const Utils::Key &settingsKey);

// Writes \a container's aspect data plus the \a useGlobal flag to the per-project
// store named \a settingsKey. While the settings are still pristine (global, and
// nothing stored yet) nothing is written, to keep fresh project files small.
PROJECTEXPLORER_EXPORT void saveProjectSettings(const Utils::AspectContainer &container,
                                                const UseGlobalAspect &useGlobal,
                                                Project *project,
                                                const Utils::Key &settingsKey);

// Lazily creates and caches a runtime-only per-project settings object. The
// object is parented to \a project, so it is dropped together with the project,
// and cached in the project's extra data for fast repeated access. \c T must
// provide a \c T(Project *) constructor and a \c static Utils::Key
// T::extraDataKey() function.
template <class T>
T *projectSettings(Project *project)
{
    const Utils::Key key = T::extraDataKey();
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        auto * const settings = new T(project);
        settings->setParent(project);
        v = QVariant::fromValue(settings);
        project->setExtraData(key, v);
    }
    return v.value<T *>();
}

} // namespace ProjectExplorer
