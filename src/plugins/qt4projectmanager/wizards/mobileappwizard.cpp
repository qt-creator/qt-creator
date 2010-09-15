/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "mobileappwizard.h"
#include "mobileappwizardpages.h"
#include "mobileapp.h"

#include "qt4projectmanagerconstants.h"

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtGui/QIcon>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
const QString DisplayName
    = QCoreApplication::translate("MobileAppWizard", "Mobile Qt Application");
const QString Description
    = QCoreApplication::translate("MobileAppWizard",
        "Creates a Qt application optimized for mobile devices "
        "with a Qt Designer-based main window.\n\n"
        "Preselects Qt for Simulator and mobile targets if available.");
}

class MobileAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT

public:
    explicit MobileAppWizardDialog(QWidget *parent = 0);

private:
    class MobileAppWizardOptionsPage *m_optionsPage;
    friend class MobileAppWizard;
};

MobileAppWizardDialog::MobileAppWizardDialog(QWidget *parent)
    : ProjectExplorer::BaseProjectWizardDialog(parent)
{
    setWindowTitle(DisplayName);
    setIntroDescription(Description);
    m_optionsPage = new MobileAppWizardOptionsPage;
    const int optionsPagePageId = addPage(m_optionsPage);
    wizardProgress()->item(optionsPagePageId)->setTitle(tr("Application options"));
}

class MobileAppWizardPrivate
{
    class MobileApp *mobileApp;
    class MobileAppWizardDialog *wizardDialog;
    friend class MobileAppWizard;
};

MobileAppWizard::MobileAppWizard()
    : Core::BaseFileWizard(parameters())
    , m_d(new MobileAppWizardPrivate)
{
    m_d->mobileApp = new MobileApp;
    m_d->wizardDialog = 0;
}

MobileAppWizard::~MobileAppWizard()
{
    delete m_d->mobileApp;
    delete m_d;
}

Core::BaseFileWizardParameters MobileAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)));
    parameters.setDisplayName(DisplayName);
    parameters.setId(QLatin1String("C.Qt4GuiMobile"));
    parameters.setDescription(Description);
    parameters.setCategory(QLatin1String(Constants::QT_APP_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QT_APP_WIZARD_TR_SCOPE,
                                                              Constants::QT_APP_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *MobileAppWizard::createWizardDialog(QWidget *parent,
    const QString &defaultPath, const WizardPageList &extensionPages) const
{
    m_d->wizardDialog = new MobileAppWizardDialog(parent);

    m_d->wizardDialog->setPath(defaultPath);
    m_d->wizardDialog->setProjectName(MobileAppWizardDialog::uniqueProjectName(defaultPath));
    m_d->wizardDialog->m_optionsPage->setSymbianSvgIcon(m_d->mobileApp->symbianSvgIcon());
    m_d->wizardDialog->m_optionsPage->setMaemoPngIcon(m_d->mobileApp->maemoPngIcon());
    m_d->wizardDialog->m_optionsPage->setOrientation(m_d->mobileApp->orientation());
    m_d->wizardDialog->m_optionsPage->setNetworkEnabled(m_d->mobileApp->networkEnabled());
    connect(m_d->wizardDialog, SIGNAL(introPageLeft(QString, QString)), SLOT(useProjectPath(QString, QString)));
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(m_d->wizardDialog, m_d->wizardDialog->addPage(p));

    return m_d->wizardDialog;
}

Core::GeneratedFiles MobileAppWizard::generateFiles(const QWizard *w,
                                                           QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const MobileAppWizardDialog *wizard = qobject_cast<const MobileAppWizardDialog*>(w);

    m_d->mobileApp->setProjectName(wizard->projectName());
    m_d->mobileApp->setProjectPath(wizard->path());
    m_d->mobileApp->setSymbianTargetUid(wizard->m_optionsPage->symbianUid());
    m_d->mobileApp->setSymbianSvgIcon(wizard->m_optionsPage->symbianSvgIcon());
    m_d->mobileApp->setOrientation(wizard->m_optionsPage->orientation());
    m_d->mobileApp->setNetworkEnabled(wizard->m_optionsPage->networkEnabled());

    return m_d->mobileApp->generateFiles(errorMessage);
}

bool MobileAppWizard::postGenerateFiles(const QWizard *wizard, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(wizard)
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

void MobileAppWizard::useProjectPath(const QString &projectName, const QString &projectPath)
{
    m_d->wizardDialog->m_optionsPage->setSymbianUid(MobileApp::symbianUidForPath(projectPath + projectName));
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "mobileappwizard.moc"
