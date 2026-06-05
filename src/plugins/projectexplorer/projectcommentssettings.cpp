// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectcommentssettings.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectpanelfactory.h"

#include <texteditor/commentssettings.h>
#include <texteditor/texteditorconstants.h>

#include <utils/layoutbuilder.h>

using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer::Internal {

const char kUseGlobalKey[] = "UseGlobalKey";

class ProjectCommentsSettings : public CommentsSettings
{
public:
    explicit ProjectCommentsSettings(Project *project)
        : m_project(project)
    {
        useGlobalSettings.setDefaultValue(true);

        const QVariant entry = project->namedSettings(CommentsSettings::mainSettingsKey());
        if (entry.isValid()) {
            const Store store = storeFromVariant(entry);
            fromMap(store);
            useGlobalSettings.setValue(store.value(kUseGlobalKey, true).toBool());
        }

        setAutoApply(true);
        setEnabled(!useGlobalSettings());

        useGlobalSettings.addOnChanged(this, [this] {
            setEnabled(!useGlobalSettings());
            save();
        });
        addOnChanged(this, [this] {
            if (!useGlobalSettings())
                save();
        });
    }

    void save()
    {
        // Optimization: Don't save if user never switched away from the default.
        if (useGlobalSettings() && !m_project->namedSettings(CommentsSettings::mainSettingsKey()).isValid())
            return;
        Store data;
        data.insert(kUseGlobalKey, useGlobalSettings());
        if (!useGlobalSettings())
            toMap(data);
        m_project->setNamedSettings(CommentsSettings::mainSettingsKey(), variantFromStore(data));
    }

    Utils::BoolAspect useGlobalSettings; // not {this}: excluded from toMap/fromMap

private:
    Project * const m_project;
};

static ProjectCommentsSettings *projectCommentsSettings(Project *project)
{
    const Key key = "ProjectCommentsSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(new ProjectCommentsSettings(project));
        project->setExtraData(key, v);
    }
    return v.value<ProjectCommentsSettings *>();
}

class ProjectCommentsSettingsWidget final : public QWidget
{
public:
    ProjectCommentsSettingsWidget(Project *project)
    {
        ProjectCommentsSettings * const ps = projectCommentsSettings(project);
        using namespace Layouting;
        Column {
            Row { ps->useGlobalSettings,
                  createUseGlobalSettingsLabel(TextEditor::Constants::TEXT_EDITOR_COMMENTS_SETTINGS),
                  st },
            createHr(),
            *ps,
            noMargin,
        }.attachTo(this);
    }
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
    const auto *ps = projectCommentsSettings(project);
    return ps->useGlobalSettings() ? globalCommentsSettings().data() : ps->data();
}
