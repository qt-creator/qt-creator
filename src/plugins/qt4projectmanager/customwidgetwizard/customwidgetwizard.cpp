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

#include "customwidgetwizard.h"
#include "customwidgetwizarddialog.h"
#include "plugingenerator.h"
#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtsupportconstants.h>

#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetWizard::CustomWidgetWizard() :
    QtWizard(QLatin1String("P.Qt4CustomWidget"),
             QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY),
             tr("Qt Custom Designer Widget"),
             tr("Creates a Qt Custom Designer Widget or a Custom Widget Collection."),
             QIcon(QLatin1String(":/wizards/images/gui.png")))
{
}

Core::FeatureSet CustomWidgetWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_QWIDGETS);
}

QWizard *CustomWidgetWizard::createWizardDialog(QWidget *parent,
                                                const Core::WizardDialogParameters &wizardDialogParameters) const
{
    CustomWidgetWizardDialog *rc = new CustomWidgetWizardDialog(displayName(),
                                                                icon(),
                                                                parent,
                                                                wizardDialogParameters);
    rc->setProjectName(CustomWidgetWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
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
} // namespace Qt4ProjectManager
