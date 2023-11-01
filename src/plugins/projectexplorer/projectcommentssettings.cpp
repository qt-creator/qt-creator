// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectcommentssettings.h"

#include "project.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QCheckBox>
#include <QVBoxLayout>

using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const char kUseGlobalKey[] = "UseGlobalKey";

ProjectCommentsSettings::ProjectCommentsSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    loadSettings();
}

CommentsSettings::Data ProjectCommentsSettings::settings() const
{
    return m_useGlobalSettings ? CommentsSettings::data() : m_customSettings;
}

void ProjectCommentsSettings::setSettings(const CommentsSettings::Data &settings)
{
    if (settings == m_customSettings)
        return;
    m_customSettings = settings;
    saveSettings();
    emit TextEditorSettings::instance()->commentsSettingsChanged();
}

void ProjectCommentsSettings::setUseGlobalSettings(bool useGlobal)
{
    if (useGlobal == m_useGlobalSettings)
        return;
    m_useGlobalSettings = useGlobal;
    saveSettings();
    emit TextEditorSettings::instance()->commentsSettingsChanged();
}

void ProjectCommentsSettings::loadSettings()
{
    if (!m_project)
        return;

    const QVariant entry = m_project->namedSettings(CommentsSettings::mainSettingsKey());
    if (!entry.isValid())
        return;

    const Store data = storeFromVariant(entry);
    m_useGlobalSettings = data.value(kUseGlobalKey, true).toBool();
    m_customSettings.enableDoxygen = data.value(CommentsSettings::enableDoxygenSettingsKey(),
                                                m_customSettings.enableDoxygen).toBool();
    m_customSettings.generateBrief = data.value(CommentsSettings::generateBriefSettingsKey(),
                                                m_customSettings.generateBrief).toBool();
    m_customSettings.leadingAsterisks = data.value(CommentsSettings::leadingAsterisksSettingsKey(),
                                                   m_customSettings.leadingAsterisks).toBool();
    m_customSettings.commandPrefix = static_cast<CommentsSettings::CommandPrefix>(
        data.value(CommentsSettings::commandPrefixKey(),
                   int(m_customSettings.commandPrefix)).toInt());
}

void ProjectCommentsSettings::saveSettings()
{
    if (!m_project)
        return;

    // Optimization: Don't save anything if the user never switched away from the default.
    if (m_useGlobalSettings && !m_project->namedSettings
                                (CommentsSettings::mainSettingsKey()).isValid()) {
        return;
    }

    Store data;
    data.insert(kUseGlobalKey, m_useGlobalSettings);
    data.insert(CommentsSettings::enableDoxygenSettingsKey(), m_customSettings.enableDoxygen);
    data.insert(CommentsSettings::generateBriefSettingsKey(), m_customSettings.generateBrief);
    data.insert(CommentsSettings::leadingAsterisksSettingsKey(), m_customSettings.leadingAsterisks);
    data.insert(CommentsSettings::commandPrefixKey(), int(m_customSettings.commandPrefix));
    m_project->setNamedSettings(CommentsSettings::mainSettingsKey(), variantFromStore(data));
}

class ProjectCommentsSettingsWidget::Private
{
public:
    Private(ProjectExplorer::Project *project) : settings(project) {}

    ProjectCommentsSettings settings;
    CommentsSettingsWidget widget{settings.settings()};
    QCheckBox useGlobalSettingsCheckBox;
};

ProjectCommentsSettingsWidget::ProjectCommentsSettingsWidget(ProjectExplorer::Project *project)
    : d(new Private(project))
{
    setGlobalSettingsId(TextEditor::Constants::TEXT_EDITOR_COMMENTS_SETTINGS);
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&d->widget);

    const auto updateGlobalSettingsCheckBox = [this] {
        setUseGlobalSettingsCheckBoxEnabled(true);
        setUseGlobalSettings(d->settings.useGlobalSettings());
        d->widget.setEnabled(!useGlobalSettings());
    };
    updateGlobalSettingsCheckBox();
    connect(TextEditorSettings::instance(), &TextEditorSettings::commentsSettingsChanged,
            this, updateGlobalSettingsCheckBox);
    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
            [this](bool checked) {
                d->widget.setEnabled(!checked);
                d->settings.setUseGlobalSettings(checked);
                if (!checked)
                    d->settings.setSettings(d->widget.settingsData());
            });
    connect(&d->widget, &CommentsSettingsWidget::settingsChanged, this, [this] {
        d->settings.setSettings(d->widget.settingsData());
    });
}

ProjectCommentsSettingsWidget::~ProjectCommentsSettingsWidget() { delete d; }

} // namespace Internal
} // namespace ProjectExplorer
