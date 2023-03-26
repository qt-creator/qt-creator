// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subdirsprojectwizarddialog.h"
#include "../qmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/filepath.h>

namespace QmakeProjectManager {
namespace Internal {

SubdirsProjectWizardDialog::SubdirsProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                       const QString &templateName,
                                                       const QIcon &icon, QWidget *parent,
                                                       const Core::WizardDialogParameters &parameters) :
    BaseQmakeProjectWizardDialog(factory, parent, parameters)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);

    setIntroDescription(Tr::tr("This wizard generates a Qt Subdirs project. "
                               "Add subprojects to it later on by using the other wizards."));

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        addTargetSetupPage();

    addExtensionPages(extensionPages());
}

QtProjectParameters SubdirsProjectWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::EmptyProject;
    rc.fileName = projectName();
    rc.path = filePath();
    return rc;
}

} // namespace Internal
} // namespace QmakeProjectManager
