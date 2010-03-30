/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <projectexplorer/customwizard/customwizard.h>

#include <coreplugin/basefilewizard.h>

#include <QtCore/QSet>

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class ModulesPage;
class TargetSetupPage;

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

    static bool qt4ProjectPostGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

protected:
    static bool showModulesPageForApplications();
    static bool showModulesPageForLibraries();

private:
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

// A custom wizard with an additional Qt 4 target page
class CustomQt4ProjectWizard : public ProjectExplorer::CustomProjectWizard {
    Q_OBJECT
public:
    explicit CustomQt4ProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                    QObject *parent = 0);

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;
    static void registerSelf();

protected:
    virtual bool postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage);

private:
    enum { targetPageId = 2 };
};

/* BaseQt4ProjectWizardDialog: Additionally offers modules page
 * and getter/setter for blank-delimited modules list, transparently
 * handling the visibility of the modules page list as well as a page
 * to select targets and Qt versions.
 */

class BaseQt4ProjectWizardDialog : public ProjectExplorer::BaseProjectWizardDialog {
    Q_OBJECT
protected:
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage,
                                        Utils::ProjectIntroPage *introPage,
                                        int introId = -1,
                                        QWidget *parent = 0);
public:
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent = 0);
    virtual ~BaseQt4ProjectWizardDialog();

    int addModulesPage(int id = -1);
    int addTargetSetupPage(QSet<QString> targets = QSet<QString>(), bool mobile = false, int id = -1);

    static QSet<QString> desktopTarget();

    QString selectedModules() const;
    void setSelectedModules(const QString &, bool lock = false);

    QString deselectedModules() const;
    void setDeselectedModules(const QString &);

    bool writeUserFile(const QString &proFileName) const;
    bool setupProject(Qt4Project *project) const;
    bool isTargetSelected(const QString &targetid) const;

private:
    inline void init(bool showModulesPage);

    ModulesPage *m_modulesPage;
    TargetSetupPage *m_targetSetupPage;
    QString m_selectedModules;
    QString m_deselectedModules;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTWIZARD_H
