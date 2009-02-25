/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "guiappwizarddialog.h"
#include "consoleappwizard.h"
#include "modulespage.h"
#include "filespage.h"
#include "qtprojectparameters.h"

#include <utils/projectintropage.h>

#include <QtGui/QAbstractButton>

enum PageId { IntroPageId, ModulesPageId, FilesPageId };

namespace Qt4ProjectManager {
namespace Internal {

GuiAppParameters::GuiAppParameters()
    : designerForm(true)
{
}

GuiAppWizardDialog::GuiAppWizardDialog(const QString &templateName,
                                       const QIcon &icon,
                                       const QList<QWizardPage*> &extensionPages,
                                       QWidget *parent) :
    QWizard(parent),
    m_introPage(new  Core::Utils::ProjectIntroPage),
    m_modulesPage(new ModulesPage),
    m_filesPage(new FilesPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    Core::BaseFileWizard::setupWizard(this);

    m_introPage->setDescription(tr("This wizard generates a Qt4 GUI application "
         "project. The application derives by default from QApplication "
         "and includes an empty widget."));
    setPage(IntroPageId, m_introPage);

    const QString coreModule = QLatin1String("core");
    const QString guiModule = QLatin1String("gui");
    m_modulesPage->setModuleSelected(coreModule);
    m_modulesPage->setModuleEnabled(coreModule, false);
    m_modulesPage->setModuleSelected(guiModule);
    m_modulesPage->setModuleEnabled(guiModule, false);
    setPage(ModulesPageId, m_modulesPage);

    m_filesPage->setFormInputCheckable(true);
    setPage(FilesPageId, m_filesPage);

    foreach (QWizardPage *p, extensionPages)
        addPage(p);
}

void GuiAppWizardDialog::setBaseClasses(const QStringList &baseClasses)
{
    m_filesPage->setBaseClassChoices(baseClasses);
    if (!baseClasses.empty())
        m_filesPage->setBaseClassName(baseClasses.front());
}

void GuiAppWizardDialog::setSuffixes(const QString &header, const QString &source, const QString &form)
{
    m_filesPage->setSuffixes(header, source, form);
}

void GuiAppWizardDialog::setPath(const QString &path)
{
    m_introPage->setPath(path);
}

void GuiAppWizardDialog::setName(const QString &name)
{
    m_introPage->setName(name);
}

QtProjectParameters GuiAppWizardDialog::projectParameters() const
{
    QtProjectParameters rc;
    rc.type =  QtProjectParameters::GuiApp;
    rc.name = m_introPage->name();
    rc.path = m_introPage->path();
    rc.selectedModules =  m_modulesPage->selectedModules();
    rc.deselectedModules = m_modulesPage-> deselectedModules();
    return rc;
}

GuiAppParameters GuiAppWizardDialog::parameters() const
{
    GuiAppParameters rc;
    rc.className = m_filesPage->className();
    rc.baseClassName = m_filesPage->baseClassName();
    rc.sourceFileName = m_filesPage->sourceFileName();
    rc.headerFileName = m_filesPage->headerFileName();
    rc.formFileName = m_filesPage->formFileName();
    rc.designerForm =  m_filesPage->formInputChecked();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
