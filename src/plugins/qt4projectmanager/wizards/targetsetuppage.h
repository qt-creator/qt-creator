/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TARGETSETUPPAGE_H
#define TARGETSETUPPAGE_H

#include "../qt4projectmanager_global.h"
#include "../qt4targetsetupwidget.h"

#include <coreplugin/featureprovider.h>
#include <projectexplorer/profilemanager.h>
#include <qtsupport/qtversionmanager.h>

#include <QString>
#include <QWizardPage>

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {
class ImportWidget;

namespace Ui {
class TargetSetupPage;
} // namespace Ui
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
    void setRequiredProfileMatcher(ProjectExplorer::ProfileMatcher *matcher);
    void setPreferredProfileMatcher(ProjectExplorer::ProfileMatcher *matcher);
    void setImportSearch(bool b);

    /// Sets whether the targetsetupage uses a scrollarea
    /// to host the widgets from the factories
    /// call this before \sa initializePage()
    void setUseScrollArea(bool b);

    bool isComplete() const;
    bool setupProject(Qt4ProjectManager::Qt4Project *project);
    bool isProfileSelected(Core::Id id) const;
    void setProfileSelected(Core::Id id, bool selected);
    bool isQtPlatformSelected(const QString &type) const;
    void setProFilePath(const QString &dir);

    /// Overrides the summary text of the targetsetuppage
    void setNoteText(const QString &text);
signals:
    void noteTextLinkActivated();

private slots:
    void import(const Utils::FileName &path);
    void handleQtUpdate(const QList<int> &add, const QList<int> &rm, const QList<int> &mod);
    void handleProfileAddition(ProjectExplorer::Profile *p);
    void handleProfileRemoval(ProjectExplorer::Profile *p);
    void handleProfileUpdate(ProjectExplorer::Profile *p);
    void updateVisibility();

private:
    void selectAtLeastOneTarget();
    void import(const Utils::FileName &path, const bool silent);
    void removeWidget(ProjectExplorer::Profile *p);
    Qt4TargetSetupWidget *addWidget(ProjectExplorer::Profile *p);

    void setupImports();

    void setupWidgets();
    void reset();

    ProjectExplorer::ProfileMatcher *m_requiredMatcher;
    ProjectExplorer::ProfileMatcher *m_preferredMatcher;
    QLayout *m_baseLayout;
    bool m_importSearch;
    bool m_useScrollArea;
    QString m_proFilePath;
    QString m_defaultShadowBuildLocation;
    QMap<Core::Id, Qt4TargetSetupWidget *> m_widgets;
    Qt4TargetSetupWidget *m_firstWidget;

    Internal::Ui::TargetSetupPage *m_ui;

    Internal::ImportWidget *m_importWidget;
    QSpacerItem *m_spacer;
};

} // namespace Qt4ProjectManager

#endif // TARGETSETUPPAGE_H
