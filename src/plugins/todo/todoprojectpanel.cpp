// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoprojectpanel.h"

#include "constants.h"
#include "todoitemsprovider.h"
#include "todotr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/useglobalaspect.h>

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QGroupBox>
#include <QVariant>

using namespace ProjectExplorer;
using namespace Utils;

namespace Todo::Internal {

class TodoProjectSettings : public AspectContainer
{
public:
    explicit TodoProjectSettings(Project *project)
        : m_project(project)
    {
        setAutoApply(true);
        excludePatterns.setUiAllowAdding(true);
        excludePatterns.setUiAllowRemoving(true);
        excludePatterns.setUiAllowEditing(true);
        excludePatterns.setToolTip(
            Tr::tr("Regular expressions for file paths to be excluded from scanning."));

        const QVariantMap s =
            project->namedSettings(Constants::SETTINGS_NAME_KEY).toMap();
        useGlobalSettings.setValue(s.value(Constants::USE_GLOBAL_KEY, true).toBool());
        excludePatterns.setValue(s.value(Constants::EXCLUDES_LIST_KEY).toStringList());

        setEnabled(!useGlobalSettings());

        useGlobalSettings.addOnChanged(this, [this] {
            setEnabled(!useGlobalSettings());
            save();
            todoItemsProvider().projectSettingsChanged();
        });
        addOnChanged(this, [this] {
            if (!useGlobalSettings())
                save();
            todoItemsProvider().projectSettingsChanged();
        });
    }

    void save()
    {
        QVariantMap s;
        s[Constants::USE_GLOBAL_KEY] = useGlobalSettings();
        if (!useGlobalSettings())
            s[Constants::EXCLUDES_LIST_KEY] = QVariant(excludePatterns());
        m_project->setNamedSettings(Constants::SETTINGS_NAME_KEY, s);
    }

    UseGlobalAspect useGlobalSettings{Constants::TODO_SETTINGS};
    StringListAspect excludePatterns{this};

private:
    Project * const m_project;
};

static TodoProjectSettings *todoProjectSettings(Project *project)
{
    const Key key = "TodoProjectSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(new TodoProjectSettings(project));
        project->setExtraData(key, v);
    }
    return v.value<TodoProjectSettings *>();
}

class TodoProjectPanelWidget final : public QWidget
{
public:
    explicit TodoProjectPanelWidget(Project *project)
    {
        TodoProjectSettings *ps = todoProjectSettings(project);

        QGroupBox *excludesGroup = nullptr;
        using namespace Layouting;
        Column {
            ps->useGlobalSettings,
            Group {
                bindTo(&excludesGroup),
                title(Tr::tr("Excluded Files")),
                Column { &ps->excludePatterns },
            },
        }.attachTo(this);

        // The QGroupBox itself is not part of the aspect container, so
        // its enabled state must be managed explicitly.
        excludesGroup->setEnabled(!ps->useGlobalSettings());
        ps->useGlobalSettings.addOnChanged(excludesGroup, [ps, excludesGroup] {
            excludesGroup->setEnabled(!ps->useGlobalSettings());
        });
    }
};

class TodoProjectPanelFactory final : public ProjectPanelFactory
{
public:
    TodoProjectPanelFactory()
    {
        setPriority(100);
        setDisplayName(Tr::tr("To-Do"));
        setCreateWidgetFunction([](Project *project) {
            return new TodoProjectPanelWidget(project);
        });
    }
};

void setupTodoProjectPanel()
{
    static TodoProjectPanelFactory theTodoProjectPanelFactory;
}

} // Todo::Internal
