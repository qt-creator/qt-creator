/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "projectexplorer_export.h"

#include "kitinformation.h"
#include "kitmanager.h"
#include "projectimporter.h"

#include <utils/wizardpage.h>

#include <QPointer>
#include <QString>
#include <QMap>

QT_FORWARD_DECLARE_CLASS(QSpacerItem)

namespace Core { class Id; }
namespace Utils { class FileName; }

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
    void setRequiredKitPredicate(const ProjectExplorer::Kit::Predicate &predicate);
    void setPreferredKitPredicate(const ProjectExplorer::Kit::Predicate &predicate);

    /// Sets whether the targetsetupage uses a scrollarea
    /// to host the widgets from the factories
    /// call this before \sa initializePage()
    void setUseScrollArea(bool b);

    bool isComplete() const override;
    bool setupProject(Project *project);
    bool isKitSelected(Core::Id id) const;
    void setKitSelected(Core::Id id, bool selected);
    QList<Core::Id> selectedKits() const;
    void setProjectPath(const QString &dir);
    void setProjectImporter(ProjectImporter *importer);

    /// Overrides the summary text of the targetsetuppage
    void setNoteText(const QString &text);
    void showOptionsHint(bool show);

    void openOptions();
    void changeAllKitsSelections();

    void kitFilterChanged(const QString &filterText);

private:
    void handleKitAddition(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);
    void handleKitUpdate(ProjectExplorer::Kit *k);
    void updateVisibility();

    void kitSelectionChanged();
    static QList<Kit *> sortedKitList(const Kit::Predicate &predicate);

    bool isUpdating() const;
    void selectAtLeastOneKit();
    void removeWidget(Kit *k);
    Internal::TargetSetupWidget *addWidget(Kit *k);

    void setupImports();
    void import(const Utils::FileName &path, bool silent = false);

    void setupWidgets();
    void reset();

    ProjectExplorer::Kit::Predicate m_requiredPredicate;
    ProjectExplorer::Kit::Predicate m_preferredPredicate;
    QPointer<ProjectImporter> m_importer = nullptr;
    QLayout *m_baseLayout = nullptr;
    QString m_projectPath;
    QString m_defaultShadowBuildLocation;
    QMap<Core::Id, Internal::TargetSetupWidget *> m_widgets;
    Internal::TargetSetupWidget *m_firstWidget = nullptr;

    Internal::TargetSetupPageUi *m_ui;

    Internal::ImportWidget *m_importWidget;
    QSpacerItem *m_spacer;
    QList<QWidget *> m_potentialWidgets;

    bool m_forceOptionHint = false;
};

} // namespace ProjectExplorer
