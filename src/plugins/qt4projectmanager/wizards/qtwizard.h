/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QTWIZARD_H
#define QTWIZARD_H

#include "qtprojectparameters.h"
#include <projectexplorer/baseprojectwizarddialog.h>

#include <coreplugin/basefilewizard.h>


namespace Qt4ProjectManager {
namespace Internal {

class ModulesPage;
class TargetsPage;

/* Base class for wizard creating Qt projects using QtProjectParameters.
 * To implement a project wizard, overwrite:
 * - createWizardDialog() to create up the dialog
 * - generateFiles() to set their contents
 * The base implementation provides the wizard parameters and opens
 * the finished project in postGenerateFiles().
 * The pro-file must be the last one of the generated files. */

class QtWizard : public Core::BaseFileWizard
{
    Q_OBJECT
    Q_DISABLE_COPY(QtWizard)

protected:
    QtWizard(const QString &id,
             const QString &category,
             const QString &categoryTranslationScope,
             const QString &displayCategory,
             const QString &name,
             const QString &description,
             const QIcon &icon);

public:

    static QString templateDir();

    static QString sourceSuffix();
    static QString headerSuffix();
    static QString formSuffix();
    static QString profileSuffix();

    // Query CppTools settings for the class wizard settings
    static bool lowerCaseFiles();

protected:
    static bool showModulesPageForApplications();
    static bool showModulesPageForLibraries();

private:
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

/* BaseQt4ProjectWizardDialog: Additionally offers modules page
 * and getter/setter for blank-delimited modules list, transparently
 * handling the visibility of the modules page list as well as a page
 * to select targets and Qt versions.
 */

class BaseQt4ProjectWizardDialog : public ProjectExplorer::BaseProjectWizardDialog {
    Q_OBJECT

protected:
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent = 0);
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage,
                                        Utils::ProjectIntroPage *introPage,
                                        int introId = -1,
                                        QWidget *parent = 0);
    virtual ~BaseQt4ProjectWizardDialog();

    void addModulesPage(int id = -1);
    void addTargetsPage(int id = -1);

public:
    QString selectedModules() const;
    void setSelectedModules(const QString &, bool lock = false);

    QString deselectedModules() const;
    void setDeselectedModules(const QString &);

    QSet<QString> selectedTargets() const;
    QList<int> selectedQtVersionIdsForTarget(const QString &target) const;

private:
    inline void init(bool showModulesPage);

    ModulesPage *m_modulesPage;
    TargetsPage *m_targetsPage;
    QString m_selectedModules;
    QString m_deselectedModules;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTWIZARD_H
