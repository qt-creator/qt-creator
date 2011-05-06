/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "customwidgetwizard.h"
#include "customwidgetwizarddialog.h"
#include "plugingenerator.h"
#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetWizard::CustomWidgetWizard() :
    QtWizard(QLatin1String("P.Qt4CustomWidget"),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_SCOPE),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY),
             tr("Qt Custom Designer Widget"),
             tr("Creates a Qt Custom Designer Widget or a Custom Widget Collection."),
             QIcon(QLatin1String(":/wizards/images/gui.png")))
{
}

QWizard *CustomWidgetWizard::createWizardDialog(QWidget *parent,
                                                const QString &defaultPath,
                                                const WizardPageList &extensionPages) const
{
    CustomWidgetWizardDialog *rc = new CustomWidgetWizardDialog(displayName(), icon(), extensionPages, parent);
    rc->setPath(defaultPath);
    rc->setProjectName(CustomWidgetWizardDialog::uniqueProjectName(defaultPath));
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
