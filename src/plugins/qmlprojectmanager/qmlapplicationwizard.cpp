/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlapplicationwizard.h"

#include "qmlapp.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qtsupport/qtkitinformation.h>

#include "qmlprojectmanager.h"
#include "qmlproject.h"
#include "qmlapplicationwizardpages.h"

#include <QIcon>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace QmlProjectManager {
namespace Internal {

QmlApplicationWizardDialog::QmlApplicationWizardDialog(QWidget *parent, const WizardDialogParameters &parameters)
    : BaseProjectWizardDialog(parent, parameters)
{
    setWindowTitle(tr("New Qt Quick UI Project"));
    setIntroDescription(tr("This wizard generates a Qt Quick UI project."));
    m_componentSetPage = new QmlComponentSetPage;
    addPage(m_componentSetPage);
}

TemplateInfo QmlApplicationWizardDialog::templateInfo() const
{
    return m_componentSetPage->templateInfo();
}


QmlApplicationWizard::QmlApplicationWizard()
    : m_qmlApp(new QmlApp(this))
{
    setWizardKind(ProjectWizard);
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    setId(QLatin1String("QA.QMLB Application"));
    setIcon(QIcon(QLatin1String(QmakeProjectManager::Constants::ICON_QTQUICK_APP)));
    setDisplayCategory(
         QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Qt Quick UI"));
    setDescription(tr("Creates a Qt Quick UI project."));
}

Core::BaseFileWizard *QmlApplicationWizard::create(QWidget *parent, const WizardDialogParameters &parameters) const
{
    QmlApplicationWizardDialog *wizardDialog = new QmlApplicationWizardDialog(parent, parameters);

    connect(wizardDialog, SIGNAL(projectParametersChanged(QString,QString)), m_qmlApp,
        SLOT(setProjectNameAndBaseDirectory(QString,QString)));

    wizardDialog->setPath(parameters.defaultPath());

    wizardDialog->setProjectName(QmlApplicationWizardDialog::uniqueProjectName(parameters.defaultPath()));

    foreach (QWizardPage *page, parameters.extensionPages())
        wizardDialog->addPage(page);

    return wizardDialog;
}

GeneratedFiles QmlApplicationWizard::generateFiles(const QWizard *w,
                                                   QString *errorMessage) const
{
    const QmlApplicationWizardDialog *wizard = qobject_cast<const QmlApplicationWizardDialog*>(w);
    m_qmlApp->setTemplateInfo(wizard->templateInfo());
    return m_qmlApp->generateFiles(errorMessage);
}

bool QmlApplicationWizard::postGenerateFiles(const QWizard * /*wizard*/, const GeneratedFiles &l,
    QString *errorMessage)
{
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace QmlProjectManager
