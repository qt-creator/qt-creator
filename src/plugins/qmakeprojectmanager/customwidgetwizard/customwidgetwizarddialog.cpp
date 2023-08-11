// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwidgetwizarddialog.h"
#include "customwidgetwidgetswizardpage.h"
#include "customwidgetpluginwizardpage.h"
#include "pluginoptions.h"
#include "../qmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

namespace QmakeProjectManager {
namespace Internal {

enum { IntroPageId = 0};

CustomWidgetWizardDialog::CustomWidgetWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                   const QString &templateName,
                                                   const QIcon &icon, QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) :
    BaseQmakeProjectWizardDialog(factory, parent, parameters),
    m_widgetsPage(new CustomWidgetWidgetsWizardPage),
    m_pluginPage(new CustomWidgetPluginWizardPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);

    setIntroDescription(Tr::tr("This wizard generates a Qt Designer Custom Widget "
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
