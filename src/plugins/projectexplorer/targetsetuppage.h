// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "kit.h"

#include <utils/wizardpage.h>

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class Kit;
class Project;
class ProjectImporter;

namespace Internal { class TargetSetupPagePrivate; }

/// \internal
class PROJECTEXPLORER_EXPORT TargetSetupPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit TargetSetupPage(QWidget *parent = nullptr);
    ~TargetSetupPage() override;

    /// Initializes the TargetSetupPage
    /// \note The import information is gathered in initializePage(), make sure that the right projectPath is set before
    void initializePage() override;

    // Call these before initializePage!
    void setTasksGenerator(const TasksGenerator &tasksGenerator);
    void setProjectPath(const Utils::FilePath &dir);
    void setProjectImporter(ProjectImporter *importer);
    bool importLineEditHasFocus() const;

    /// Sets whether the targetsetupage uses a scrollarea
    /// to host the widgets from the factories
    /// call this before \sa initializePage()
    void setUseScrollArea(bool b);

    bool isComplete() const override;
    bool setupProject(Project *project);
    QList<Utils::Id> selectedKits() const;

    void openOptions();
    void changeAllKitsSelections();

private:
    void showEvent(QShowEvent *event) final;

    Internal::TargetSetupPagePrivate *d;
};

} // namespace ProjectExplorer
