/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "customwidgetwizarddialog.h"
#include "customwidgetwidgetswizardpage.h"
#include "customwidgetpluginwizardpage.h"
#include "customwidgetwizard.h"

namespace Qt4ProjectManager {
namespace Internal {

enum { IntroPageId = 0};

CustomWidgetWizardDialog::CustomWidgetWizardDialog(const QString &templateName,
                                                   const QIcon &icon,
                                                   const QList<QWizardPage*> &extensionPages,
                                                   QWidget *parent) :
    BaseQt4ProjectWizardDialog(false, parent),
    m_widgetsPage(new CustomWidgetWidgetsWizardPage),
    m_pluginPage(new CustomWidgetPluginWizardPage),
    m_widgetPageId(-1), m_pluginPageId(-1)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);

    setIntroDescription(tr("This wizard generates a Qt4 Designer Custom Widget "
                           "or a Qt4 Designer Custom Widget Collection project."));

    addTargetSetupPage(BaseQt4ProjectWizardDialog::desktopTarget());
    m_widgetPageId = addPage(m_widgetsPage);
    m_pluginPageId = addPage(m_pluginPage);
    wizardProgress()->item(m_widgetPageId)->setTitle(tr("Custom Widgets"));
    wizardProgress()->item(m_pluginPageId)->setTitle(tr("Plugin Details"));

    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));
}

FileNamingParameters CustomWidgetWizardDialog::fileNamingParameters() const
{
    return m_widgetsPage->fileNamingParameters();
}

void CustomWidgetWizardDialog::setFileNamingParameters(const FileNamingParameters &fnp)
{
    m_widgetsPage->setFileNamingParameters(fnp);
    m_pluginPage->setFileNamingParameters(fnp);
}

void CustomWidgetWizardDialog::slotCurrentIdChanged(int id)
{
    if (id == m_pluginPageId)
        m_pluginPage->init(m_widgetsPage);
}

QSharedPointer<PluginOptions> CustomWidgetWizardDialog::pluginOptions() const
{
    QSharedPointer<PluginOptions> rc = m_pluginPage->basicPluginOptions();
    rc->widgetOptions = m_widgetsPage->widgetOptions();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
