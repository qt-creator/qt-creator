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
#include <projectexplorer/target.h>

static bool isMultilanguagePresent()
{
    const QVector<ExtensionSystem::PluginSpec *> specs = ExtensionSystem::PluginManager::plugins();
    return std::find_if(specs.begin(),
                        specs.end(),
                        [](ExtensionSystem::PluginSpec *spec) {
                            return spec->name() == "MultiLanguage";
                        })
           != specs.end();
}

static Utils::FilePath getMultilanguageDatabaseFilePath(ProjectExplorer::Target *target)
{
    if (target) {
        auto filePath = target->project()->projectDirectory().pathAppended("/multilanguage-experimental-v1.db");
        if (filePath.exists())
            return filePath;
    }
    return {};
}

static QObject *getPreviewPlugin()
{
    auto pluginIt = std::find_if(ExtensionSystem::PluginManager::plugins().begin(),
                                 ExtensionSystem::PluginManager::plugins().end(),
                                 [](const ExtensionSystem::PluginSpec *p) {
                                     return p->name() == "QmlPreview";
                                 });

    if (pluginIt != ExtensionSystem::PluginManager::plugins().constEnd())
        return (*pluginIt)->plugin();

    return nullptr;
}


namespace QmlProjectManager {

QmlMultiLanguageAspect::QmlMultiLanguageAspect(ProjectExplorer::Target *target)
    : m_target(target)
{
    setVisible(isMultilanguagePresent());
    setSettingsKey(Constants::USE_MULTILANGUAGE_KEY);
    setLabel(tr("Use MultiLanguage translation database."), BaseBoolAspect::LabelPlacement::AtCheckBox);
    setToolTip(tr("Enable loading application with special desktop SQLite translation database."));

    setDefaultValue(!databaseFilePath().isEmpty());
    QVariantMap getDefaultValues;
    fromMap(getDefaultValues);

    if (auto previewPlugin = getPreviewPlugin())
        connect(previewPlugin, SIGNAL(localeChanged(QString)), this, SLOT(setLastUsedLanguage(QString)));
}

QmlMultiLanguageAspect::~QmlMultiLanguageAspect()
{
}

void QmlMultiLanguageAspect::setLastUsedLanguage(const QString &language)
{
    if (auto previewPlugin = getPreviewPlugin())
        previewPlugin->setProperty("locale", language);
    if (m_lastUsedLanguage != language) {
        m_lastUsedLanguage = language;
        emit changed();
    }
}

QString QmlMultiLanguageAspect::lastUsedLanguage() const
{
    return m_lastUsedLanguage;
}

Utils::FilePath QmlMultiLanguageAspect::databaseFilePath() const
{
    if (m_databaseFilePath.isEmpty())
        m_databaseFilePath = getMultilanguageDatabaseFilePath(m_target);
    return m_databaseFilePath;
}

void QmlMultiLanguageAspect::toMap(QVariantMap &map) const
{
    BaseBoolAspect::toMap(map);
    if (!m_lastUsedLanguage.isEmpty())
        map.insert(Constants::LAST_USED_LANGUAGE, m_lastUsedLanguage);
}

void QmlMultiLanguageAspect::fromMap(const QVariantMap &map)
{
    BaseBoolAspect::fromMap(map);
    setLastUsedLanguage(map.value(Constants::LAST_USED_LANGUAGE, "en").toString());
}

} // namespace QmlProjectManager
