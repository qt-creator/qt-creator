/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "guiappwizarddialog.h"

#include "filespage.h"
#include "qt4target.h"
#include "qtprojectparameters.h"
#include "qt4projectmanagerconstants.h"

#include <QtCore/QSet>

namespace Qt4ProjectManager {
namespace Internal {

GuiAppParameters::GuiAppParameters()
    : designerForm(true),
      isMobileApplication(false)
{
}

GuiAppWizardDialog::GuiAppWizardDialog(const QString &templateName,
                                       const QIcon &icon,
                                       const QList<QWizardPage*> &extensionPages,
                                       bool showModulesPage,
                                       bool isMobile,
                                       QWidget *parent) :
    BaseQt4ProjectWizardDialog(showModulesPage, parent),
    m_filesPage(new FilesPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core gui"), true);

    setIntroDescription(tr("This wizard generates a Qt4 GUI application "
         "project. The application derives by default from QApplication "
         "and includes an empty widget."));

    addModulesPage();
    addTargetSetupPage(QSet<QString>(), isMobile);

    m_filesPage->setFormInputCheckable(true);
    m_filesPage->setClassTypeComboVisible(false);
    const int filesPageId = addPage(m_filesPage);
    wizardProgress()->item(filesPageId)->setTitle(tr("Details"));

    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
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
    rc.fileName = projectName();
    rc.path = path();
    rc.selectedModules = selectedModules();
    rc.deselectedModules = deselectedModules();
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
    if (isTargetSelected(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
            || isTargetSelected(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
            || isTargetSelected(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))) {
        rc.widgetWidth = 800;
        rc.widgetHeight = 480;
    } else if (isTargetSelected(QLatin1String(Constants::S60_DEVICE_TARGET_ID)) ||
               isTargetSelected(QLatin1String(Constants::S60_EMULATOR_TARGET_ID))) {
        rc.widgetWidth = 360;
        rc.widgetHeight = 640;
    } else {
        rc.isMobileApplication = false;
        rc.widgetWidth = 400;
        rc.widgetHeight = 300;
    }
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
