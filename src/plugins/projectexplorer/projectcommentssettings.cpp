// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectcommentssettings.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"

#include <texteditor/commentssettings.h>
#include <texteditor/texteditorconstants.h>

#include <utils/layoutbuilder.h>

#include <QVBoxLayout>

using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer::Internal {

const char kUseGlobalKey[] = "UseGlobalKey";

class ProjectCommentsSettingsWidget final : public ProjectSettingsWidget
{
public:
    ProjectCommentsSettingsWidget(Project *project)
        : m_project(project)
    {
        setGlobalSettingsId(TextEditor::Constants::TEXT_EDITOR_COMMENTS_SETTINGS);

        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *inner = new QWidget;
        layout->addWidget(inner);

        bool useGlobal = true;
        if (project) {
            const QVariant entry = project->namedSettings(CommentsSettings::mainSettingsKey());
            if (entry.isValid()) {
                const Store store = storeFromVariant(entry);
                useGlobal = store.value(kUseGlobalKey, true).toBool();
                if (!useGlobal)
                    m_displayedSettings.fromMap(store);
            }
        }
        if (useGlobal)
            m_displayedSettings.copyFrom(globalCommentsSettings());
        m_displayedSettings.setAutoApply(true);
        m_displayedSettings.layouter()().attachTo(inner);

        setUseGlobalSettings(useGlobal);
        inner->setEnabled(!useGlobal);

        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
                [this, inner](bool useGlobal) {
                    inner->setEnabled(!useGlobal);
                    if (useGlobal)
                        m_displayedSettings.copyFrom(globalCommentsSettings());
                    saveSettings(useGlobal);
                });

        connect(&globalCommentsSettings(), &CommentsSettings::changed,
                this, [this] {
                    if (useGlobalSettings())
                        m_displayedSettings.copyFrom(globalCommentsSettings());
                });

        connect(&m_displayedSettings, &AspectContainer::changed, this, [this] {
            if (!useGlobalSettings())
                saveSettings(false);
        });
    }

private:
    void saveSettings(bool useGlobal)
    {
        if (!m_project)
            return;
        if (useGlobal && !m_project->namedSettings(CommentsSettings::mainSettingsKey()).isValid())
            return;
        Store data;
        data.insert(kUseGlobalKey, useGlobal);
        if (!useGlobal)
            m_displayedSettings.toMap(data);
        m_project->setNamedSettings(CommentsSettings::mainSettingsKey(), variantFromStore(data));
    }

    Project * const m_project;
    CommentsSettings m_displayedSettings;
};

class CommentsSettingsProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CommentsSettingsProjectPanelFactory()
    {
        setPriority(45);
        setDisplayName(Tr::tr("Documentation Comments"));
        setCreateWidgetFunction([](Project *project) {
            return new ProjectCommentsSettingsWidget(project);
        });
    }
};

void setupCommentsSettingsProjectPanel()
{
    static CommentsSettingsProjectPanelFactory theCommentsSettingsProjectPanelFactory;
}

} // namespace ProjectExplorer::Internal

TextEditor::CommentsSettings::Data ProjectExplorer::commentsSettings(const FilePath &filePath)
{
    using namespace Internal;
    Project * const project = ProjectManager::projectForFile(filePath);
    if (!project)
        return globalCommentsSettings().data();

    const QVariant entry = project->namedSettings(CommentsSettings::mainSettingsKey());
    if (!entry.isValid())
        return globalCommentsSettings().data();

    const Store data = storeFromVariant(entry);
    if (data.value(kUseGlobalKey, true).toBool())
        return globalCommentsSettings().data();

    TextEditor::CommentsSettings::Data customSettings;
    customSettings.fromMap(data);
    return customSettings;
}
