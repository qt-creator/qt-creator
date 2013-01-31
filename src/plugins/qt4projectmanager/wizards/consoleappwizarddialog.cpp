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

#include "consoleappwizarddialog.h"
#include "consoleappwizard.h"
#include <coreplugin/basefilewizard.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDebug>

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizardDialog::ConsoleAppWizardDialog(const QString &templateName,
                                               const QIcon &icon,
                                               bool showModulesPage,
                                               QWidget *parent, const Core::WizardDialogParameters &parameters) :
    BaseQt4ProjectWizardDialog(showModulesPage, parent, parameters)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core"));
    setDeselectedModules(QLatin1String("gui"));

    setIntroDescription(tr("This wizard generates a Qt console application "
                          "project. The application derives from QCoreApplication and does not "
                          "provide a GUI."));

    addModulesPage();
    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        addTargetSetupPage();

    addExtensionPages(parameters.extensionPages());
}

QtProjectParameters ConsoleAppWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::ConsoleApp;
    rc.fileName = projectName();
    rc.path = path();

    rc.selectedModules = selectedModulesList();
    rc.deselectedModules = deselectedModulesList();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
