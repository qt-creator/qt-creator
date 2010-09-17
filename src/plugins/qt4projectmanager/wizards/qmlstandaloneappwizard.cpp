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

#include "mobileappwizardpages.h"
#include "qmlstandaloneapp.h"
#include "qmlstandaloneappwizard.h"
#include "qmlstandaloneappwizardpages.h"

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
    explicit QmlStandaloneAppWizardDialog(QmlStandaloneAppWizard::WizardType type, QWidget *parent = 0);

private:
    QmlStandaloneAppWizard::WizardType m_type;
    class QmlStandaloneAppWizardSourcesPage *m_qmlSourcesPage;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizardDialog::QmlStandaloneAppWizardDialog(QmlStandaloneAppWizard::WizardType type,
                                                           QWidget *parent)
    : AbstractMobileAppWizardDialog(parent)
    , m_type(type)
{
    setWindowTitle(m_type == QmlStandaloneAppWizard::NewQmlFile
                       ? tr("New QML Project")
                       : tr("QML Project from existing, QML Viewer-based Project"));
    setIntroDescription(m_type == QmlStandaloneAppWizard::NewQmlFile
                       ? tr("This wizard generates a QML application project.")
                       : tr("This wizard imports an existing, QML Viewer-based application and creates a standalone version of it."));

    m_qmlSourcesPage = new QmlStandaloneAppWizardSourcesPage;
    m_qmlSourcesPage->setMainQmlFileChooserVisible(m_type == QmlStandaloneAppWizard::ImportQmlFile);
    const int qmlSourcesPagePageId = addPage(m_qmlSourcesPage);
    wizardProgress()->item(qmlSourcesPagePageId)->setTitle(tr("QML Sources"));
}

class QmlStandaloneAppWizardPrivate
{
    QmlStandaloneAppWizard::WizardType type;
    class QmlStandaloneApp *standaloneApp;
    class QmlStandaloneAppWizardDialog *wizardDialog;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizard::QmlStandaloneAppWizard(WizardType type)
    : AbstractMobileAppWizard(parameters(type))
    , m_d(new QmlStandaloneAppWizardPrivate)
{
    m_d->type = type;
    m_d->standaloneApp = new QmlStandaloneApp;
    m_d->wizardDialog = 0;
}

QmlStandaloneAppWizard::~QmlStandaloneAppWizard()
{
    delete m_d->standaloneApp;
    delete m_d;
}

Core::BaseFileWizardParameters QmlStandaloneAppWizard::parameters(WizardType type)
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QML_STANDALONE)));
    parameters.setDisplayName(type == QmlStandaloneAppWizard::NewQmlFile
                              ? tr("Qt QML New Application")
                              : tr("Qt QML New Imported Application"));
    parameters.setId(QLatin1String(type == QmlStandaloneAppWizard::NewQmlFile
                                   ? "QA.QMLA New Application"
                                   : "QA.QMLB Imported Application"));
    parameters.setDescription(type == QmlStandaloneAppWizard::NewQmlFile
                              ? tr("Creates a mobile-deployable Qt QML application "
                                   "project. A lightweight Qt/C++ application with a QDeclarativeView "
                                   "and a single QML file will be created.")
                              : tr("Creates a mobile-deployable Qt QML application "
                                   "project. An existing, QML Viewer-based project will be imported and a lightweight "
                                   "Qt/C++ application with a QDeclarativeView will be created for it."));
    parameters.setCategory(QLatin1String(Constants::QT_APP_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QT_APP_WIZARD_TR_SCOPE,
                                                              Constants::QT_APP_WIZARD_TR_CATEGORY));
    return parameters;
}

AbstractMobileAppWizardDialog *QmlStandaloneAppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new QmlStandaloneAppWizardDialog(m_d->type, parent);
    connect(m_d->wizardDialog->m_qmlSourcesPage,
            SIGNAL(externalModulesChanged(QStringList, QStringList)), SLOT(handleModulesChange(QStringList, QStringList)));
    return m_d->wizardDialog;
}

void QmlStandaloneAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QmlStandaloneAppWizardDialog *wizard = qobject_cast<const QmlStandaloneAppWizardDialog*>(w);
    if (m_d->type == QmlStandaloneAppWizard::ImportQmlFile)
        m_d->standaloneApp->setMainQmlFile(wizard->m_qmlSourcesPage->mainQmlFile());
    m_d->standaloneApp->setExternalModules(
            wizard->m_qmlSourcesPage->moduleUris(), wizard->m_qmlSourcesPage->moduleImportPaths());
}

bool QmlStandaloneAppWizard::postGenerateFiles(const QWizard *wizard, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(wizard)
    const bool success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
    if (success && m_d->type == QmlStandaloneAppWizard::ImportQmlFile) {
        ProjectExplorer::ProjectExplorerPlugin::instance()->setCurrentFile(0, m_d->standaloneApp->mainQmlFile());
        Core::EditorManager::instance()->openEditor(m_d->standaloneApp->mainQmlFile(),
                                                    QString(), Core::EditorManager::ModeSwitch);
    }
    return success;
}

void QmlStandaloneAppWizard::handleModulesChange(const QStringList &uris, const QStringList &paths)
{
    QmlStandaloneApp testApp;
    testApp.setExternalModules(uris, paths);
    m_d->wizardDialog->m_qmlSourcesPage->setModulesError(testApp.error());
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
