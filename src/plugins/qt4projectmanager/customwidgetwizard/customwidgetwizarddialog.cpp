/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "customwidgetwizarddialog.h"
#include "customwidgetwidgetswizardpage.h"
#include "customwidgetpluginwizardpage.h"
#include "customwidgetwizard.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

namespace {

class DesktopQtKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    bool matches(const ProjectExplorer::Kit *k) const
    {
        ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(k);
        if (dev.isNull() || dev->id() != ProjectExplorer::Constants::DESKTOP_DEVICE_ID)
            return false;
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        return version && version->type() == QtSupport::Constants::DESKTOPQT;
    }
};

} // namespace

namespace Qt4ProjectManager {
namespace Internal {

enum { IntroPageId = 0};

CustomWidgetWizardDialog::CustomWidgetWizardDialog(const QString &templateName,
                                                   const QIcon &icon,
                                                   QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) :
    BaseQt4ProjectWizardDialog(false, parent, parameters),
    m_widgetsPage(new CustomWidgetWidgetsWizardPage),
    m_pluginPage(new CustomWidgetPluginWizardPage),
    m_widgetPageId(-1), m_pluginPageId(-1)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);

    setIntroDescription(tr("This wizard generates a Qt4 Designer Custom Widget "
                           "or a Qt4 Designer Custom Widget Collection project."));

    if (!parameters.extraValues().contains(ProjectExplorer::Constants::PROJECT_PROFILE_IDS))
        addTargetSetupPage();
    m_widgetPageId = addPage(m_widgetsPage);
    m_pluginPageId = addPage(m_pluginPage);
    wizardProgress()->item(m_widgetPageId)->setTitle(tr("Custom Widgets"));
    wizardProgress()->item(m_pluginPageId)->setTitle(tr("Plugin Details"));

    addExtensionPages(parameters.extensionPages());
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
