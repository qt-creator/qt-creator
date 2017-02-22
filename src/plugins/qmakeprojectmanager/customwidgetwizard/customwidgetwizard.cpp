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

#include "customwidgetwizard.h"
#include "customwidgetwizarddialog.h"
#include "plugingenerator.h"
#include "filenamingparameters.h"
#include "pluginoptions.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>

namespace QmakeProjectManager {
namespace Internal {

CustomWidgetWizard::CustomWidgetWizard()
{
    setId("P.Qt4CustomWidget");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
             ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Qt Custom Designer Widget"));
    setDescription(tr("Creates a Qt Custom Designer Widget or a Custom Widget Collection."));
    setIcon(QIcon(QLatin1String(":/wizards/images/gui.png")));
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
    const CustomWidgetWizardDialog *cw = qobject_cast<const CustomWidgetWizardDialog *>(w);
    Q_ASSERT(w);
    GenerationParameters p;
    p.fileName = cw->projectName();
    p.path = cw->path();
    p.templatePath = QtWizard::templateDir();
    p.templatePath += QLatin1String("/customwidgetwizard");
    return PluginGenerator::generatePlugin(p, *(cw->pluginOptions()), errorMessage);
}

} // namespace Internal
} // namespace QmakeProjectManager
