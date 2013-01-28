/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TARGETSETUPPAGE_H
#define TARGETSETUPPAGE_H

#include "../qt4projectmanager_global.h"
#include "../qt4targetsetupwidget.h"

#include <coreplugin/featureprovider.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/qtversionmanager.h>

#include <QString>
#include <QWizardPage>

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {
class ImportWidget;
class TargetSetupPageUi;
} // namespace Internal

/// \internal
class QT4PROJECTMANAGER_EXPORT TargetSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit TargetSetupPage(QWidget* parent = 0);
    ~TargetSetupPage();

    /// Initializes the TargetSetupPage
    /// \note The import information is gathered in initializePage(), make sure that the right proFilePath is set before
    void initializePage();

    // Call these before initializePage!
    void setRequiredKitMatcher(ProjectExplorer::KitMatcher *matcher);
    void setPreferredKitMatcher(ProjectExplorer::KitMatcher *matcher);
    void setImportSearch(bool b);

    /// Sets whether the targetsetupage uses a scrollarea
    /// to host the widgets from the factories
    /// call this before \sa initializePage()
    void setUseScrollArea(bool b);

    bool isComplete() const;
    bool setupProject(Qt4ProjectManager::Qt4Project *project);
    bool isKitSelected(Core::Id id) const;
    void setKitSelected(Core::Id id, bool selected);
    QList<Core::Id> selectedKits() const;
    void setProFilePath(const QString &dir);

    /// Overrides the summary text of the targetsetuppage
    void setNoteText(const QString &text);
    void showOptionsHint(bool show);

private slots:
    void import(const Utils::FileName &path);
    void handleQtUpdate(const QList<int> &add, const QList<int> &rm, const QList<int> &mod);
    void handleKitAddition(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);
    void handleKitUpdate(ProjectExplorer::Kit *k);
    void updateVisibility();
    void openOptions();

private:
    void selectAtLeastOneKit();
    void import(const Utils::FileName &path, const bool silent);
    void removeWidget(ProjectExplorer::Kit *k);
    Qt4TargetSetupWidget *addWidget(ProjectExplorer::Kit *k);

    void setupImports();

    void setupWidgets();
    void reset();
    ProjectExplorer::Kit *createTemporaryKit(QtSupport::BaseQtVersion *version, bool temporaryVersion, const Utils::FileName &parsedSpec);
    void cleanKit(ProjectExplorer::Kit *k);
    void makeQtPersistent(ProjectExplorer::Kit *k);
    void addProject(ProjectExplorer::Kit *k, const QString &path);
    void removeProject(ProjectExplorer::Kit *k, const QString &path);

    ProjectExplorer::KitMatcher *m_requiredMatcher;
    ProjectExplorer::KitMatcher *m_preferredMatcher;
    QLayout *m_baseLayout;
    bool m_importSearch;
    bool m_ignoreUpdates;
    QString m_proFilePath;
    QString m_defaultShadowBuildLocation;
    QMap<Core::Id, Qt4TargetSetupWidget *> m_widgets;
    Qt4TargetSetupWidget *m_firstWidget;

    Internal::TargetSetupPageUi *m_ui;

    Internal::ImportWidget *m_importWidget;
    QSpacerItem *m_spacer;

    bool m_forceOptionHint;
};

} // namespace Qt4ProjectManager

#endif // TARGETSETUPPAGE_H
