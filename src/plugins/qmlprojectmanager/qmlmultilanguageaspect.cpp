/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmlmultilanguageaspect.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

static bool isMultilanguagePresent()
{
    const QVector<ExtensionSystem::PluginSpec *> &specs = ExtensionSystem::PluginManager::plugins();
    return std::find_if(specs.cbegin(), specs.cend(),
                        [](ExtensionSystem::PluginSpec *spec) {
                            return spec->name() == "MultiLanguage";
                        })
           != specs.cend();
}

static Utils::FilePath getMultilanguageDatabaseFilePath(ProjectExplorer::Target *target)
{
    if (target) {
        auto filePath = target->project()->projectDirectory().pathAppended(
            "multilanguage-experimental-v5.db");
        if (filePath.exists())
            return filePath;
    }
    return {};
}

static QObject *getPreviewPlugin()
{
    const QVector<ExtensionSystem::PluginSpec *> &specs = ExtensionSystem::PluginManager::plugins();
    const auto pluginIt = std::find_if(specs.cbegin(), specs.cend(),
                                 [](const ExtensionSystem::PluginSpec *p) {
                                     return p->name() == "QmlPreview";
                                 });

    if (pluginIt != specs.cend())
        return (*pluginIt)->plugin();

    return nullptr;
}


namespace QmlProjectManager {

QmlMultiLanguageAspect::QmlMultiLanguageAspect(ProjectExplorer::Target *target)
    : m_target(target)
{
    setVisible(isMultilanguagePresent());
    setSettingsKey(Constants::USE_MULTILANGUAGE_KEY);
    setLabel(tr("Use MultiLanguage translation database."), BoolAspect::LabelPlacement::AtCheckBox);
    setToolTip(tr("Enable loading application with special desktop SQLite translation database."));

    setDefaultValue(!databaseFilePath().isEmpty());
    QVariantMap getDefaultValues;
    fromMap(getDefaultValues);
}

QmlMultiLanguageAspect::~QmlMultiLanguageAspect()
{
}

void QmlMultiLanguageAspect::setCurrentLocale(const QString &locale)
{
    if (m_currentLocale == locale)
        return;
    m_currentLocale = locale;
    if (auto previewPlugin = getPreviewPlugin())
        previewPlugin->setProperty("localeIsoCode", locale);
}

QString QmlMultiLanguageAspect::currentLocale() const
{
    return m_currentLocale;
}

Utils::FilePath QmlMultiLanguageAspect::databaseFilePath() const
{
    if (m_databaseFilePath.isEmpty())
        m_databaseFilePath = getMultilanguageDatabaseFilePath(m_target);
    return m_databaseFilePath;
}

void QmlMultiLanguageAspect::toMap(QVariantMap &map) const
{
    BoolAspect::toMap(map);
    if (!m_currentLocale.isEmpty())
        map.insert(Constants::LAST_USED_LANGUAGE, m_currentLocale);
}

void QmlMultiLanguageAspect::fromMap(const QVariantMap &map)
{
    BoolAspect::fromMap(map);
    setCurrentLocale(map.value(Constants::LAST_USED_LANGUAGE, "en").toString());
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current()
{
    if (auto project = ProjectExplorer::SessionManager::startupProject())
        return current(project);
    return {};
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current(ProjectExplorer::Project *project)
{
    if (auto target = project->activeTarget())
        return current(target);
    return {};
}

QmlMultiLanguageAspect *QmlMultiLanguageAspect::current(ProjectExplorer::Target *target)
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
