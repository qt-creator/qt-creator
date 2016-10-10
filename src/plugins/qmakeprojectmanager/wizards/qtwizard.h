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

#include "qtprojectparameters.h"
#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>

namespace ProjectExplorer {
class Kit;
class TargetSetupPage;
} // namespace ProjectExplorer

namespace QmakeProjectManager {

class QmakeProject;

namespace Internal {

class ModulesPage;

/* Base class for wizard creating Qt projects using QtProjectParameters.
 * To implement a project wizard, overwrite:
 * - createWizardDialog() to create up the dialog
 * - generateFiles() to set their contents
 * The base implementation provides the wizard parameters and opens
 * the finished project in postGenerateFiles().
 * The pro-file must be the last one of the generated files. */

class QtWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

protected:
    QtWizard();

public:
    static QString templateDir();

    static QString sourceSuffix();
    static QString headerSuffix();
    static QString formSuffix();
    static QString profileSuffix();

    // Query CppTools settings for the class wizard settings
    static bool lowerCaseFiles();

    static bool qt4ProjectPostGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

private:
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;
};

// A custom wizard with an additional Qt 4 target page
class CustomQmakeProjectWizard : public ProjectExplorer::CustomProjectWizard
{
    Q_OBJECT

public:
    CustomQmakeProjectWizard();

private:
    Core::BaseFileWizard *create(QWidget *parent,
                                 const Core::WizardDialogParameters &parameters) const override;
    bool postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;

private:
    enum { targetPageId = 1 };
};

/* BaseQmakeProjectWizardDialog: Additionally offers modules page
 * and getter/setter for blank-delimited modules list, transparently
 * handling the visibility of the modules page list as well as a page
 * to select targets and Qt versions.
 */

class BaseQmakeProjectWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
protected:
    explicit BaseQmakeProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                          bool showModulesPage,
                                          Utils::ProjectIntroPage *introPage,
                                          int introId, QWidget *parent,
                                          const Core::WizardDialogParameters &parameters);
public:
    explicit BaseQmakeProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                          bool showModulesPage, QWidget *parent,
                                          const Core::WizardDialogParameters &parameters);
    ~BaseQmakeProjectWizardDialog() override;

    int addModulesPage(int id = -1);
    int addTargetSetupPage(int id = -1);

    QStringList selectedModulesList() const;
    void setSelectedModules(const QString &, bool lock = false);

    QStringList deselectedModulesList() const;
    void setDeselectedModules(const QString &);

    bool writeUserFile(const QString &proFileName) const;
    bool isQtPlatformSelected(Core::Id platform) const;
    QList<Core::Id> selectedKits() const;

    void addExtensionPages(const QList<QWizardPage *> &wizardPageList);

private:
    void generateProfileName(const QString &name, const QString &path);

    inline void init(bool showModulesPage);

    ModulesPage *m_modulesPage;
    ProjectExplorer::TargetSetupPage *m_targetSetupPage;
    QStringList m_selectedModules;
    QStringList m_deselectedModules;
    QList<Core::Id> m_profileIds;
};

} // namespace Internal
} // namespace QmakeProjectManager
