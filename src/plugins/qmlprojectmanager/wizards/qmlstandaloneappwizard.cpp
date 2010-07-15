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

#include "qmlstandaloneappwizard.h"
#include "qmlstandaloneappwizardoptionspage.h"
#include "qmlstandaloneapp.h"

#include "qmlprojectconstants.h"

#include <projectexplorer/customwizard/customwizard.h>

#include <QtGui/QIcon>

#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>

namespace QmlProjectManager {
namespace Internal {

class QmlNewStandaloneAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT

public:
    explicit QmlNewStandaloneAppWizardDialog(QWidget *parent = 0);

private:
    class QmlStandaloneAppWizardOptionPage *m_qmlOptionsPage;
    friend class QmlNewStandaloneAppWizard;
};

QmlNewStandaloneAppWizardDialog::QmlNewStandaloneAppWizardDialog(QWidget *parent)
    : ProjectExplorer::BaseProjectWizardDialog(parent)
{
    setWindowTitle(tr("New Standalone QML Project"));
    setIntroDescription(tr("This wizard generates a Standalone QML application project."));

    m_qmlOptionsPage = new QmlStandaloneAppWizardOptionPage;

    const int qmlOptionsPagePageId = addPage(m_qmlOptionsPage);
    wizardProgress()->item(qmlOptionsPagePageId)->setTitle(tr("Qml App options"));
}

QmlNewStandaloneAppWizard::QmlNewStandaloneAppWizard()
    : Core::BaseFileWizard(parameters())
    , m_standaloneApp(new QmlStandaloneApp)
    , m_wizardDialog(0)
{
}

QmlNewStandaloneAppWizard::~QmlNewStandaloneAppWizard()
{
    delete m_standaloneApp;
}

Core::BaseFileWizardParameters QmlNewStandaloneAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::QML_WIZARD_ICON)));
    parameters.setDisplayName(tr("Qt QML Standalone Application"));
    parameters.setId(QLatin1String("QA.QML Standalone Application"));
    parameters.setDescription(tr("Creates a standalone, mobile-deployable Qt QML application "
                                 "project. A lightweight Qt/C++ application with a QDeclarativeView "
                                 "and a single QML file will be created."));
    parameters.setCategory(QLatin1String(Constants::QML_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QML_WIZARD_TR_SCOPE,
                                                              Constants::QML_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *QmlNewStandaloneAppWizard::createWizardDialog(QWidget *parent,
                                                       const QString &defaultPath,
                                                       const WizardPageList &extensionPages) const
{
    m_wizardDialog = new QmlNewStandaloneAppWizardDialog(parent);

    m_wizardDialog->setPath(defaultPath);
    m_wizardDialog->setProjectName(QmlNewStandaloneAppWizardDialog::uniqueProjectName(defaultPath));
    m_wizardDialog->m_qmlOptionsPage->setSymbianSvgIcon(m_standaloneApp->symbianSvgIcon());
    m_wizardDialog->m_qmlOptionsPage->setOrientation(m_standaloneApp->orientation());
    m_wizardDialog->m_qmlOptionsPage->setNetworkEnabled(m_standaloneApp->networkEnabled());
    m_wizardDialog->m_qmlOptionsPage->setLoadDummyData(m_standaloneApp->loadDummyData());
    connect(m_wizardDialog, SIGNAL(introPageLeft(QString, QString)), SLOT(useProjectPath(QString, QString)));

    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(m_wizardDialog, m_wizardDialog->addPage(p));

    return m_wizardDialog;
}

Core::GeneratedFiles QmlNewStandaloneAppWizard::generateFiles(const QWizard *w,
                                                              QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QmlNewStandaloneAppWizardDialog *wizard = qobject_cast<const QmlNewStandaloneAppWizardDialog*>(w);

    m_standaloneApp->setProjectName(wizard->projectName());
    m_standaloneApp->setProjectPath(wizard->path());
    m_standaloneApp->setSymbianTargetUid(wizard->m_qmlOptionsPage->symbianUid());
    m_standaloneApp->setSymbianSvgIcon(wizard->m_qmlOptionsPage->symbianSvgIcon());
    m_standaloneApp->setOrientation(wizard->m_qmlOptionsPage->orientation());
    m_standaloneApp->setNetworkEnabled(wizard->m_qmlOptionsPage->networkEnabled());

    return m_standaloneApp->generateFiles(errorMessage);
}

bool QmlNewStandaloneAppWizard::postGenerateFiles(const QWizard *wizard, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(wizard)
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

void QmlNewStandaloneAppWizard::useProjectPath(const QString &projectName, const QString &projectPath)
{
    m_wizardDialog->m_qmlOptionsPage->setSymbianUid(QmlStandaloneApp::symbianUidForPath(projectPath + projectName));
}

} // namespace Internal
} // namespace QmlProjectManager

#include "qmlstandaloneappwizard.moc"
