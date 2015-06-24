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

#include "subdirsprojectwizard.h"

#include "subdirsprojectwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/icore.h>
#include <qtsupport/qtsupportconstants.h>

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
    setRequiredFeatures(Core::FeatureSet(Core::Feature::versionedFeature(QtSupport::Constants::FEATURE_QT_PREFIX)));
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
        Core::ICore::showNewItemDialog(tr("New Subproject", "Title of dialog"),
                              Core::IWizardFactory::wizardFactoriesOfKind(Core::IWizardFactory::ProjectWizard),
                              wizard->parameters().projectPath(),
                              map);
    } else {
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace QmakeProjectManager
