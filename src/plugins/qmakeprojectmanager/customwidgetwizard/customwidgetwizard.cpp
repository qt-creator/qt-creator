// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwidgetwizard.h"
#include "customwidgetwizarddialog.h"
#include "plugingenerator.h"
#include "filenamingparameters.h"
#include "pluginoptions.h"
#include "../qmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>

#include <qtsupport/qtsupportconstants.h>

#include <utils/filepath.h>

namespace QmakeProjectManager {
namespace Internal {

CustomWidgetWizard::CustomWidgetWizard()
{
    setId("P.Qt4CustomWidget");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(ProjectExplorer::Tr::tr(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(Tr::tr("Qt Custom Designer Widget"));
    setDescription(Tr::tr("Creates a Qt Custom Designer Widget or a Custom Widget Collection."));
    setIcon(themedIcon(":/wizards/images/gui.png"));
    setRequiredFeatures({QtSupport::Constants::FEATURE_QWIDGETS});
}

Core::BaseFileWizard *CustomWidgetWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    CustomWidgetWizardDialog *rc = new CustomWidgetWizardDialog(this, displayName(),
                                                                icon(), parent, parameters);
    rc->setProjectName(CustomWidgetWizardDialog::uniqueProjectName(parameters.defaultPath()));
    rc->setFileNamingParameters(FileNamingParameters(headerSuffix(), sourceSuffix(), QtWizard::lowerCaseFiles()));
    return rc;
}

Core::GeneratedFiles CustomWidgetWizard::generateFiles(const QWizard *w,
                                                       QString *errorMessage) const
{
    const auto *cw = qobject_cast<const CustomWidgetWizardDialog *>(w);
    Q_ASSERT(w);
    GenerationParameters p;
    p.fileName = cw->projectName();
    p.path = cw->filePath().toString();
    p.templatePath = QtWizard::templateDir();
    p.templatePath += QLatin1String("/customwidgetwizard");
    return PluginGenerator::generatePlugin(p, *(cw->pluginOptions()), errorMessage);
}

} // namespace Internal
} // namespace QmakeProjectManager
