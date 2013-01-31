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

#include "subdirsprojectwizard.h"

#include "subdirsprojectwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/icore.h>

#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {

SubdirsProjectWizard::SubdirsProjectWizard()
  : QtWizard(QLatin1String("U.Qt4Subdirs"),
             QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY),
             tr("Subdirs Project"),
             tr("Creates a qmake-based subdirs project. This allows you to group "
                "your projects in a tree structure."),
             QIcon(QLatin1String(":/wizards/images/gui.png")))
{
}

QWizard *SubdirsProjectWizard::createWizardDialog(QWidget *parent,
                                                  const Core::WizardDialogParameters &wizardDialogParameters) const
{
    SubdirsProjectWizardDialog *dialog = new SubdirsProjectWizardDialog(displayName(), icon(), parent, wizardDialogParameters);

    dialog->setProjectName(SubdirsProjectWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
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
        const QtProjectParameters params = wizard->parameters();
        const QString projectPath = params.projectPath();
        const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, params.fileName, profileSuffix());
        QVariantMap map;
        map.insert(QLatin1String(ProjectExplorer::Constants::PREFERED_PROJECT_NODE), profileName);
        map.insert(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS), QVariant::fromValue(wizard->selectedKits()));
        Core::ICore::showNewItemDialog(tr("New Subproject", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard),
                              wizard->parameters().projectPath(),
                              map);
    } else {
        return false;
    }
    return true;
}

Core::FeatureSet SubdirsProjectWizard::requiredFeatures() const
{
    return Core::FeatureSet();
}

} // namespace Internal
} // namespace Qt4ProjectManager
