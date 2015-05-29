/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
} // namespace QmakeProjectManager
