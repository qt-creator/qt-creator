/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "consoleappwizarddialog.h"
#include "consoleappwizard.h"
#include "modulespage.h"

#include <utils/projectintropage.h>

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizardDialog::ConsoleAppWizardDialog(const QString &templateName,
                                               const QIcon &icon,
                                               const QList<QWizardPage*> &extensionPages,
                                               QWidget *parent) :
    QWizard(parent),
    m_introPage(new  Core::Utils::ProjectIntroPage),
    m_modulesPage(new ModulesPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    Core::BaseFileWizard::setupWizard(this);
    setOptions(QWizard::IndependentPages | QWizard::HaveNextButtonOnLastPage);

    m_introPage->setDescription(tr("This wizard generates a Qt4 console application "
                          "project. The application derives from QCoreApplication and does not "
                          "present a GUI. You can press 'Finish' at any point in time."));

    m_introPage->setFinalPage(true);
    addPage(m_introPage);

    m_modulesPage->setModuleSelected(QLatin1String("core"));
    addPage(m_modulesPage);
    foreach (QWizardPage *p, extensionPages)
        addPage(p);
}

void ConsoleAppWizardDialog::setPath(const QString &path)
{
    m_introPage->setPath(path);
}

void  ConsoleAppWizardDialog::setName(const QString &name)
{
    m_introPage->setName(name);
}

QtProjectParameters ConsoleAppWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::ConsoleApp;
    rc.name = m_introPage->name();
    rc.path = m_introPage->path();
    rc.selectedModules =  m_modulesPage->selectedModules();
    rc.deselectedModules = m_modulesPage-> deselectedModules();
    return rc;
}

}
}
