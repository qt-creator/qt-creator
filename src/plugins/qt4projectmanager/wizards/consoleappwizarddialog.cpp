/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "consoleappwizarddialog.h"
#include "consoleappwizard.h"

#include <QtCore/QDebug>

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizardDialog::ConsoleAppWizardDialog(const QString &templateName,
                                               const QIcon &icon,
                                               const QList<QWizardPage*> &extensionPages,
                                               bool showModulesPage,
                                               QWidget *parent) :
    BaseQt4ProjectWizardDialog(showModulesPage, parent)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core"));

    setIntroDescription(tr("This wizard generates a Qt4 console application "
                          "project. The application derives from QCoreApplication and does not "
                          "provide a GUI."));

    addModulesPage();
    foreach (QWizardPage *p, extensionPages)
        addPage(p);
}

QtProjectParameters ConsoleAppWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::ConsoleApp;
    rc.name = name();
    rc.path = path();

    rc.selectedModules = selectedModules();
    rc.deselectedModules = deselectedModules();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
