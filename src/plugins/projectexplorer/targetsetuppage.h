/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TARGETSETUPPAGE_H
#define TARGETSETUPPAGE_H

#include "projectexplorer_export.h"

#include "projectimporter.h"
#include "kitinformation.h"

#include <utils/wizardpage.h>

#include <QString>
#include <QMap>

QT_FORWARD_DECLARE_CLASS(QSpacerItem)

namespace Core { class Id; }
namespace Utils { class FileName; }

namespace ProjectExplorer {
class Kit;
class KitMatcher;
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
    explicit TargetSetupPage(QWidget *parent = 0);
    ~TargetSetupPage();

    /// Initializes the TargetSetupPage
    /// \note The import information is gathered in initializePage(), make sure that the right projectPath is set before
    void initializePage();

    // Call these before initializePage!
    void setRequiredKitMatcher(const KitMatcher &matcher);
    void setPreferredKitMatcher(const KitMatcher &matcher);

    /// Sets whether the targetsetupage uses a scrollarea
    /// to host the widgets from the factories
    /// call this before \sa initializePage()
    void setUseScrollArea(bool b);

    bool isComplete() const;
    bool setupProject(Project *project);
    bool isKitSelected(Core::Id id) const;
    void setKitSelected(Core::Id id, bool selected);
    QList<Core::Id> selectedKits() const;
    void setProjectPath(const QString &dir);
    void setProjectImporter(ProjectImporter *importer);

    /// Overrides the summary text of the targetsetuppage
    void setNoteText(const QString &text);
    void showOptionsHint(bool show);

private slots:
    void handleKitAddition(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);
    void handleKitUpdate(ProjectExplorer::Kit *k);
    void updateVisibility();
    void openOptions();
    void import(const Utils::FileName &path);

    void kitSelectionChanged();
    void changeAllKitsSelections();

private:
    bool isUpdating() const;
    void selectAtLeastOneKit();
    void removeWidget(Kit *k);
    Internal::TargetSetupWidget *addWidget(Kit *k);

    void setupImports();
    void import(const Utils::FileName &path, bool silent);

    void setupWidgets();
    void reset();

    KitMatcher m_requiredMatcher;
    KitMatcher m_preferredMatcher;
    ProjectImporter *m_importer;
    QLayout *m_baseLayout;
    QString m_projectPath;
    QString m_defaultShadowBuildLocation;
    QMap<Core::Id, Internal::TargetSetupWidget *> m_widgets;
    Internal::TargetSetupWidget *m_firstWidget;

    Internal::TargetSetupPageUi *m_ui;

    Internal::ImportWidget *m_importWidget;
    QSpacerItem *m_spacer;
    QList<QWidget *> m_potentialWidgets;

    bool m_forceOptionHint;
};

} // namespace ProjectExplorer

#endif // TARGETSETUPPAGE_H
