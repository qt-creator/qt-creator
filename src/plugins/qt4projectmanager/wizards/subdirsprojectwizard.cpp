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

#include "subdirsprojectwizard.h"

#include "subdirsprojectwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/icore.h>

#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

SubdirsProjectWizard::SubdirsProjectWizard()
  : QtWizard(QLatin1String("U.Qt4Subdirs"),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_SCOPE),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY),
             tr("Subdirs Project"),
             tr("Creates a qmake-based subdirs project. This allows you to group "
                "your projects in a tree structure."),
             QIcon(QLatin1String(":/wizards/images/gui.png")))
{
}

QWizard *SubdirsProjectWizard::createWizardDialog(QWidget *parent,
                                              const QString &defaultPath,
                                              const WizardPageList &extensionPages) const
{
    SubdirsProjectWizardDialog *dialog = new SubdirsProjectWizardDialog(displayName(), icon(), extensionPages, parent);
    dialog->setPath(defaultPath);
    dialog->setProjectName(SubdirsProjectWizardDialog::uniqueProjectName(defaultPath));
    const QString buttonText = dialog->wizardStyle() == QWizard::MacStyle
            ? tr("Done && Add Subproject") : tr("Finish && Add Subproject");
    dialog->setButtonText(QWizard::FinishButton, buttonText);
    return dialog;
}

Core::GeneratedFiles SubdirsProjectWizard::generateFiles(const QWizard *w,
                                                         QString * /*errorMessage*/) const
{
    const SubdirsProjectWizardDialog *wizard = qobject_cast< const SubdirsProjectWizardDialog *>(w);
    const QtProjectParameters params = wizard->parameters();
    const QString projectPath = params.projectPath();
    const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute | Core::GeneratedFile::OpenEditorAttribute);
    profile.setContents(QLatin1String("TEMPLATE = subdirs\n"));
    return Core::GeneratedFiles() << profile;
}

bool SubdirsProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &files, QString *errorMessage)
{
    const SubdirsProjectWizardDialog *wizard = qobject_cast< const SubdirsProjectWizardDialog *>(w);
    if (QtWizard::qt4ProjectPostGenerateFiles(wizard, files, errorMessage)) {
        Core::ICore::instance()->showNewItemDialog(tr("New Subproject", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard),
                              wizard->parameters().projectPath());
    } else {
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
