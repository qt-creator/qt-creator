// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "kitinformation.h"
#include "kitmanager.h"
#include "projectimporter.h"
#include "task.h"

#include <utils/wizardpage.h>

#include <QPointer>
#include <QString>
#include <QMap>

QT_FORWARD_DECLARE_CLASS(QSpacerItem)

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class Kit;
class Project;

namespace Internal {
class ImportWidget;
class TargetSetupPageUi;
class TargetSetupWidget;
} // namespace Internal

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

    void kitFilterChanged(const QString &filterText);

private:
    void doInitializePage();

    void showEvent(QShowEvent *event) final;

    void handleKitAddition(Kit *k);
    void handleKitRemoval(Kit *k);
    void handleKitUpdate(Kit *k);
    void updateVisibility();

    void reLayout();
    static bool compareKits(const Kit *k1, const Kit *k2);
    std::vector<Internal::TargetSetupWidget *> sortedWidgetList() const;

    void kitSelectionChanged();

    bool isUpdating() const;
    void selectAtLeastOneEnabledKit();
    void removeWidget(Kit *k) { removeWidget(widget(k)); }
    void removeWidget(Internal::TargetSetupWidget *w);
    Internal::TargetSetupWidget *addWidget(Kit *k);
    void addAdditionalWidgets();
    void removeAdditionalWidgets(QLayout *layout);
    void removeAdditionalWidgets() { removeAdditionalWidgets(m_baseLayout); }
    void updateWidget(Internal::TargetSetupWidget *widget);
    bool isUsable(const Kit *kit) const;

    void setupImports();
    void import(const Utils::FilePath &path, bool silent = false);

    void setupWidgets(const QString &filterText = QString());
    void reset();

    Internal::TargetSetupWidget *widget(const Kit *k,
                                        Internal::TargetSetupWidget *fallback = nullptr) const
    {
        return k ? widget(k->id(), fallback) : fallback;
    }
    Internal::TargetSetupWidget *widget(const Utils::Id kitId,
                                        Internal::TargetSetupWidget *fallback = nullptr) const;

    TasksGenerator m_tasksGenerator;
    QPointer<ProjectImporter> m_importer;
    QLayout *m_baseLayout = nullptr;
    Utils::FilePath m_projectPath;
    QString m_defaultShadowBuildLocation;
    std::vector<Internal::TargetSetupWidget *> m_widgets;

    Internal::TargetSetupPageUi *m_ui;

    Internal::ImportWidget *m_importWidget;
    QSpacerItem *m_spacer;
    QList<QWidget *> m_potentialWidgets;

    bool m_widgetsWereSetUp = false;
};

} // namespace ProjectExplorer
