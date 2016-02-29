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

#include "subdirsprojectwizard.h"

#include "subdirsprojectwizarddialog.h"
#include "../qmakeprojectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/icore.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>

#include <QCoreApplication>

namespace QmakeProjectManager {
namespace Internal {

SubdirsProjectWizard::SubdirsProjectWizard()
{
    setId("U.Qt4Subdirs");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Subdirs Project"));
    setDescription(tr("Creates a qmake-based subdirs project. This allows you to group "
                "your projects in a tree structure."));
    setIcon(QIcon(QLatin1String(":/wizards/images/gui.png")));
    setRequiredFeatures({ QtSupport::Constants::FEATURE_QT_PREFIX });
}

Core::BaseFileWizard *SubdirsProjectWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    SubdirsProjectWizardDialog *dialog = new SubdirsProjectWizardDialog(this, displayName(), icon(),
                                                                        parent, parameters);

    dialog->setProjectName(SubdirsProjectWizardDialog::uniqueProjectName(parameters.defaultPath()));
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
    const QString profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute | Core::GeneratedFile::OpenEditorAttribute);
    profile.setContents(QLatin1String("TEMPLATE = subdirs\n"));
    return Core::GeneratedFiles() << profile;
}

bool SubdirsProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &files,
                                             QString *errorMessage) const
{
    const SubdirsProjectWizardDialog *wizard = qobject_cast< const SubdirsProjectWizardDialog *>(w);
    if (QtWizard::qt4ProjectPostGenerateFiles(wizard, files, errorMessage)) {
        const QtProjectParameters params = wizard->parameters();
        const QString projectPath = params.projectPath();
        const QString profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, params.fileName, profileSuffix());
        QVariantMap map;
        map.insert(QLatin1String(ProjectExplorer::Constants::PREFERRED_PROJECT_NODE), profileName);
        map.insert(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS), QVariant::fromValue(wizard->selectedKits()));
        IWizardFactory::requestNewItemDialog(tr("New Subproject", "Title of dialog"),
                                             Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                                             [](Core::IWizardFactory *f) {
                                                                 return f->supportedProjectTypes().contains(Constants::QMAKEPROJECT_ID);
                                                             }),
                                             wizard->parameters().projectPath(), map);
    } else {
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace QmakeProjectManager
