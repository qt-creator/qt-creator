// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmultilanguageaspect.h"

#include "qmlprojectconstants.h"
#include "qmlprojectmanagertr.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager {

static bool isMultilanguagePresent()
{
    return ExtensionSystem::PluginManager::specExists("multilanguage");
}

static QObject *getPlugin(const QString &pluginId)
{
    const auto spec = ExtensionSystem::PluginManager::specById(pluginId);
    return spec ? spec->plugin() : nullptr;
}

QmlMultiLanguageAspect::QmlMultiLanguageAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setVisible(isMultilanguagePresent());
    setSettingsKey(Constants::USE_MULTILANGUAGE_KEY);
    setLabel(Tr::tr("Use MultiLanguage in 2D view"), BoolAspect::LabelPlacement::AtCheckBox);
    setToolTip(Tr::tr("Reads translations from MultiLanguage plugin."));

    setDefaultValue(!databaseFilePath().isEmpty());
    Store getDefaultValues;
    fromMap(getDefaultValues);

    addDataExtractor(this, &QmlMultiLanguageAspect::origin, &Data::origin);

    connect(this, &BoolAspect::changed, this, [this] {
        for (RunControl *runControl : ProjectExplorerPlugin::allRunControls()) {
            if (auto aspect = runControl->aspectData<QmlMultiLanguageAspect>()) {
                if (auto origin = aspect->origin; origin == this)
                    runControl->initiateStop();
            }
        }
    });
}

void QmlMultiLanguageAspect::setCurrentLocale(const QString &locale)
{
    if (m_currentLocale == locale)
        return;
    m_currentLocale = locale;
    if (auto previewPlugin = getPlugin("qmlpreview"))
        previewPlugin->setProperty("localeIsoCode", locale);
}

QString QmlMultiLanguageAspect::currentLocale() const
{
    return m_currentLocale;
}

Utils::FilePath QmlMultiLanguageAspect::databaseFilePath() const
{
    if (auto previewPlugin = getPlugin("multilanguage")) {
        const auto multilanguageDatabaseFilePath = previewPlugin->property("multilanguageDatabaseFilePath");
        return Utils::FilePath::fromString(multilanguageDatabaseFilePath.toString());
    }
    return {};
}

void QmlMultiLanguageAspect::toMap(Store &map) const
{
    BoolAspect::toMap(map);
    if (!m_currentLocale.isEmpty())
        map.insert(Constants::LAST_USED_LANGUAGE, m_currentLocale);
}

void QmlMultiLanguageAspect::fromMap(const Store &map)
{
    BoolAspect::fromMap(map);
    setCurrentLocale(map.value(Constants::LAST_USED_LANGUAGE, "en").toString());
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current()
{
    if (auto project = ProjectManager::startupProject())
        return current(project);
    return {};
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current(Project *project)
{
    if (auto target = project->activeTarget())
        return current(target);
    return {};
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current(Target *target)
{
    if (!target)
        return {};

    if (auto runConfiguration = target->activeRunConfiguration()) {
        if (auto multiLanguageAspect = runConfiguration->aspect<QmlProjectManager::QmlMultiLanguageAspect>())
            return multiLanguageAspect;
    }
    return {};
}

} // namespace QmlProjectManager
