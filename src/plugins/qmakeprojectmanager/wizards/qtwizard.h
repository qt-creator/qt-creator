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

#ifndef QTWIZARD_H
#define QTWIZARD_H

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
    ~BaseQmakeProjectWizardDialog();

    int addModulesPage(int id = -1);
    int addTargetSetupPage(int id = -1);

    QStringList selectedModulesList() const;
    void setSelectedModules(const QString &, bool lock = false);

    QStringList deselectedModulesList() const;
    void setDeselectedModules(const QString &);

    bool writeUserFile(const QString &proFileName) const;
    bool setupProject(QmakeProject *project) const;
    bool isQtPlatformSelected(const QString &platform) const;
    QList<Core::Id> selectedKits() const;

    void addExtensionPages(const QList<QWizardPage *> &wizardPageList);

private slots:
    void generateProfileName(const QString &name, const QString &path);

private:
    inline void init(bool showModulesPage);

    ModulesPage *m_modulesPage;
    ProjectExplorer::TargetSetupPage *m_targetSetupPage;
    QStringList m_selectedModules;
    QStringList m_deselectedModules;
    QList<Core::Id> m_profileIds;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // QTWIZARD_H
