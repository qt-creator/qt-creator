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

#include "guiappwizarddialog.h"

#include "filespage.h"
#include "qtprojectparameters.h"

#include <qtsupport/qtsupportconstants.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <QSet>

namespace Qt4ProjectManager {
namespace Internal {

GuiAppParameters::GuiAppParameters()
    : designerForm(true),
      isMobileApplication(false)
{
}

GuiAppWizardDialog::GuiAppWizardDialog(const QString &templateName,
                                       const QIcon &icon,
                                       bool showModulesPage,
                                       bool isMobile,
                                       QWidget *parent,
                                       const Core::WizardDialogParameters &parameters) :
    BaseQt4ProjectWizardDialog(showModulesPage, parent, parameters),
    m_filesPage(new FilesPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core gui"), true);

    setIntroDescription(tr("This wizard generates a Qt GUI application "
         "project. The application derives by default from QApplication "
         "and includes an empty widget."));

    addModulesPage();
    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        addTargetSetupPage(isMobile);

    m_filesPage->setFormInputCheckable(true);
    m_filesPage->setClassTypeComboVisible(false);
    const int filesPageId = addPage(m_filesPage);
    wizardProgress()->item(filesPageId)->setTitle(tr("Details"));

    addExtensionPages(parameters.extensionPages());
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

void GuiAppWizardDialog::setLowerCaseFiles(bool l)
{
    m_filesPage->setLowerCaseFiles(l);
}

QtProjectParameters GuiAppWizardDialog::projectParameters() const
{
    QtProjectParameters rc;
    rc.type =  QtProjectParameters::GuiApp;
    rc.flags |= QtProjectParameters::WidgetsRequiredFlag;
    rc.fileName = projectName();
    rc.path = path();
    rc.selectedModules = selectedModulesList();
    rc.deselectedModules = deselectedModulesList();
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
    rc.isMobileApplication = true;
    if (isQtPlatformSelected(QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM))
            || isQtPlatformSelected(QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
            || isQtPlatformSelected(QLatin1String(QtSupport::Constants::ANDROID_PLATFORM))) {
        rc.widgetWidth = 800;
        rc.widgetHeight = 480;
    } else {
        rc.isMobileApplication = false;
        rc.widgetWidth = 400;
        rc.widgetHeight = 300;
    }
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
