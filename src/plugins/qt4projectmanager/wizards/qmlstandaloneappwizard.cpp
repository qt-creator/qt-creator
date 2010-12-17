/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlstandaloneapp.h"
#include "qmlstandaloneappwizard.h"
#include "qmlstandaloneappwizardpages.h"
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

class QmlStandaloneAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QmlStandaloneAppWizardDialog(QWidget *parent = 0);

private:
    class QmlStandaloneAppWizardSourcesPage *m_qmlSourcesPage;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizardDialog::QmlStandaloneAppWizardDialog(QWidget *parent)
    : AbstractMobileAppWizardDialog(parent)
    , m_qmlSourcesPage(0)
{
    setWindowTitle(tr("New Qt Quick Application"));
    setIntroDescription(tr("This wizard generates a Qt Quick application project."));

    m_qmlSourcesPage = new QmlStandaloneAppWizardSourcesPage;
    addPageWithTitle(m_qmlSourcesPage, tr("QML Sources"));
}


class QmlStandaloneAppWizardPrivate
{
    class QmlStandaloneApp *standaloneApp;
    class QmlStandaloneAppWizardDialog *wizardDialog;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizard::QmlStandaloneAppWizard()
    : AbstractMobileAppWizard(parameters())
    , m_d(new QmlStandaloneAppWizardPrivate)
{
    m_d->standaloneApp = new QmlStandaloneApp;
    m_d->wizardDialog = 0;
}

QmlStandaloneAppWizard::~QmlStandaloneAppWizard()
{
    delete m_d->standaloneApp;
    delete m_d;
}

Core::BaseFileWizardParameters QmlStandaloneAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QML_STANDALONE)));
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

AbstractMobileAppWizardDialog *QmlStandaloneAppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new QmlStandaloneAppWizardDialog(parent);
    const QList<TargetSetupPage::ImportInfo> &qtVersions
        = TargetSetupPage::importInfosForKnownQtVersions();
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

    return m_d->wizardDialog;
}

void QmlStandaloneAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QmlStandaloneAppWizardDialog *wizard = qobject_cast<const QmlStandaloneAppWizardDialog*>(w);
    const QString mainQmlFile = wizard->m_qmlSourcesPage->mainQmlFile();
    if (!mainQmlFile.isEmpty())
        m_d->standaloneApp->setMainQmlFile(mainQmlFile);
}

bool QmlStandaloneAppWizard::postGenerateFilesInternal(const Core::GeneratedFiles &l,
    QString *errorMessage)
{
    const bool success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
    if (success && !m_d->standaloneApp->mainQmlFile().isEmpty()) {
        ProjectExplorer::ProjectExplorerPlugin::instance()->setCurrentFile(0, m_d->standaloneApp->mainQmlFile());
        Core::EditorManager::instance()->openEditor(m_d->standaloneApp->mainQmlFile(),
                                                    QString(), Core::EditorManager::ModeSwitch);
    }
    return success;
}

AbstractMobileApp *QmlStandaloneAppWizard::app() const
{
    return m_d->standaloneApp;
}

AbstractMobileAppWizardDialog *QmlStandaloneAppWizard::wizardDialog() const
{
    return m_d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "qmlstandaloneappwizard.moc"
