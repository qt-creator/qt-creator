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

#include "customwidgetwizarddialog.h"
#include "customwidgetwidgetswizardpage.h"
#include "customwidgetpluginwizardpage.h"
#include "pluginoptions.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

namespace QmakeProjectManager {
namespace Internal {

enum { IntroPageId = 0};

CustomWidgetWizardDialog::CustomWidgetWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                   const QString &templateName,
                                                   const QIcon &icon, QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) :
    BaseQmakeProjectWizardDialog(factory, false, parent, parameters),
    m_widgetsPage(new CustomWidgetWidgetsWizardPage),
    m_pluginPage(new CustomWidgetPluginWizardPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);

    setIntroDescription(tr("This wizard generates a Qt Designer Custom Widget "
                           "or a Qt Designer Custom Widget Collection project."));

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        addTargetSetupPage();
    addPage(m_widgetsPage);
    m_pluginPageId = addPage(m_pluginPage);

    addExtensionPages(extensionPages());
    connect(this, &QWizard::currentIdChanged, this, &CustomWidgetWizardDialog::slotCurrentIdChanged);
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
} // namespace QmakeProjectManager
