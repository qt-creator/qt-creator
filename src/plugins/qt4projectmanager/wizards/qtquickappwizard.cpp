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

#include "mobileappwizardpages.h"

#include "qtquickapp.h"
#include "qtquickappwizard.h"
#include "qtquickappwizardpages.h"
#include "targetsetuppage.h"

#include "qt4projectmanagerconstants.h"

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class QtQuickAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QtQuickAppWizardDialog(QWidget *parent = 0);

private:
    class QtQuickAppWizardSourcesPage *m_qmlSourcesPage;
    friend class QtQuickAppWizard;
};

QtQuickAppWizardDialog::QtQuickAppWizardDialog(QWidget *parent)
    : AbstractMobileAppWizardDialog(parent)
    , m_qmlSourcesPage(0)
{
    setWindowTitle(tr("New Qt Quick Application"));
    setIntroDescription(tr("This wizard generates a Qt Quick application project."));

    m_qmlSourcesPage = new QtQuickAppWizardSourcesPage;
    addPageWithTitle(m_qmlSourcesPage, tr("QML Sources"));
}


class QtQuickAppWizardPrivate
{
    class QtQuickApp *app;
    class QtQuickAppWizardDialog *wizardDialog;
    friend class QtQuickAppWizard;
};

QtQuickAppWizard::QtQuickAppWizard()
    : AbstractMobileAppWizard(parameters())
    , m_d(new QtQuickAppWizardPrivate)
{
    m_d->app = new QtQuickApp;
    m_d->wizardDialog = 0;
}

QtQuickAppWizard::~QtQuickAppWizard()
{
    delete m_d->app;
    delete m_d;
}

Core::BaseFileWizardParameters QtQuickAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QTQUICK_APP)));
    parameters.setDisplayName(tr("Qt Quick Application"));
    parameters.setId(QLatin1String("QA.QMLA Application"));
    parameters.setDescription(tr("Creates a Qt Quick application project that can contain "
                                 "both QML and C++ code and includes a QDeclarativeView.\n\n"
                                 "You can build the application and deploy it on desktop and "
                                 "mobile target platforms. For example, you can create signed "
                                 "Symbian Installation System (SIS) packages for this type of "
                                 "projects."));
    parameters.setCategory(QLatin1String(Constants::QML_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QML_WIZARD_TR_SCOPE,
                                                              Constants::QML_WIZARD_TR_CATEGORY));
    return parameters;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new QtQuickAppWizardDialog(parent);
    return m_d->wizardDialog;
}

void QtQuickAppWizard::projectPathChanged(const QString &path) const
{
    const QList<TargetSetupPage::ImportInfo> &qtVersions
        = TargetSetupPage::importInfosForKnownQtVersions(path);
    QList<TargetSetupPage::ImportInfo> qmlQtVersions;
    foreach (const TargetSetupPage::ImportInfo &qtVersion, qtVersions) {
        const QString versionString = qtVersion.version->qtVersionString();
        bool isNumber;
        const int majorVersion = versionString.mid(0, 1).toInt(&isNumber);
        if (!isNumber || majorVersion < 4)
            continue;
        const int minorVersion = versionString.mid(2, 1).toInt(&isNumber);
        if (!isNumber || (majorVersion == 4 && minorVersion < 7))
            continue;
        qmlQtVersions << qtVersion;
    }
    m_d->wizardDialog->m_targetsPage->setImportInfos(qmlQtVersions);
}

void QtQuickAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QtQuickAppWizardDialog *wizard = qobject_cast<const QtQuickAppWizardDialog*>(w);
    const QString mainQmlFile = wizard->m_qmlSourcesPage->mainQmlFile();
    m_d->app->setMainQmlFile(mainQmlFile);
}

bool QtQuickAppWizard::postGenerateFilesInternal(const Core::GeneratedFiles &l,
    QString *errorMessage)
{
    const bool success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
    if (success && !m_d->app->mainQmlFile().isEmpty()) {
        ProjectExplorer::ProjectExplorerPlugin::instance()->setCurrentFile(0, m_d->app->mainQmlFile());
        Core::EditorManager::instance()->openEditor(m_d->app->mainQmlFile(),
                                                    QString(), Core::EditorManager::ModeSwitch);
    }
    return success;
}

AbstractMobileApp *QtQuickAppWizard::app() const
{
    return m_d->app;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::wizardDialog() const
{
    return m_d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "qtquickappwizard.moc"
