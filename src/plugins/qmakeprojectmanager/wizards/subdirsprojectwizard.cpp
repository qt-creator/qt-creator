// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subdirsprojectwizard.h"

#include "subdirsprojectwizarddialog.h"
#include "../qmakeprojectmanagerconstants.h"
#include "../qmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>

#include <coreplugin/icore.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>

using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

SubdirsProjectWizard::SubdirsProjectWizard()
{
    setId("U.Qt4Subdirs");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(ProjectExplorer::Tr::tr(
        ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(Tr::tr("Subdirs Project"));
    setDescription(Tr::tr("Creates a qmake-based subdirs project. This allows you to group "
                          "your projects in a tree structure."));
    setIcon(themedIcon(":/wizards/images/gui.png"));
    setRequiredFeatures({QtSupport::Constants::FEATURE_QT_PREFIX});
}

Core::BaseFileWizard *SubdirsProjectWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    SubdirsProjectWizardDialog *dialog = new SubdirsProjectWizardDialog(this, displayName(), icon(),
                                                                        parent, parameters);

    dialog->setProjectName(SubdirsProjectWizardDialog::uniqueProjectName(parameters.defaultPath()));
    const QString buttonText = dialog->wizardStyle() == QWizard::MacStyle
            ? Tr::tr("Done && Add Subproject") : Tr::tr("Finish && Add Subproject");
    dialog->setButtonText(QWizard::FinishButton, buttonText);
    return dialog;
}

Core::GeneratedFiles SubdirsProjectWizard::generateFiles(const QWizard *w,
                                                         QString * /*errorMessage*/) const
{
    const auto *wizard = qobject_cast< const SubdirsProjectWizardDialog *>(w);
    const QtProjectParameters params = wizard->parameters();
    const FilePath projectPath = params.projectPath();
    const FilePath profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute | Core::GeneratedFile::OpenEditorAttribute);
    profile.setContents(QLatin1String("TEMPLATE = subdirs\n"));
    return Core::GeneratedFiles() << profile;
}

bool SubdirsProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &files,
                                             QString *errorMessage) const
{
    const auto *wizard = qobject_cast< const SubdirsProjectWizardDialog *>(w);
    if (QtWizard::qt4ProjectPostGenerateFiles(wizard, files, errorMessage)) {
        const QtProjectParameters params = wizard->parameters();
        const FilePath projectPath = params.projectPath();
        const FilePath profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, params.fileName, profileSuffix());
        QVariantMap map;
        map.insert(QLatin1String(ProjectExplorer::Constants::PREFERRED_PROJECT_NODE), profileName.toVariant());
        map.insert(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS),
                   Utils::transform<QStringList>(wizard->selectedKits(), &Utils::Id::toString));
        IWizardFactory::requestNewItemDialog(Tr::tr("New Subproject", "Title of dialog"),
                                             Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                                             [](Core::IWizardFactory *f) {
                                                                 return f->supportedProjectTypes().contains(Constants::QMAKEPROJECT_ID);
                                                             }),
                                             wizard->parameters().projectPath(),
                                             map);
    } else {
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace QmakeProjectManager
