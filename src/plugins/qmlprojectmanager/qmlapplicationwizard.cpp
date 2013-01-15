/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <coreplugin/icore.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>

#include "qmlprojectmanager.h"
#include "qmlproject.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QUrl>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace QmlProjectManager {
namespace Internal {

QmlApplicationWizardDialog::QmlApplicationWizardDialog(QmlApp *qmlApp, QWidget *parent, const WizardDialogParameters &parameters)
    : BaseProjectWizardDialog(parent, parameters),
      m_qmlApp(qmlApp)
{
}

QmlApp *QmlApplicationWizardDialog::qmlApp() const
{
    return m_qmlApp;
}

QmlApplicationWizard::QmlApplicationWizard(const BaseFileWizardParameters &parameters,
                                                   const TemplateInfo &templateInfo, QObject *parent)
    : BaseFileWizard(parameters, parent),
      m_qmlApp(new QmlApp(this))
{
    m_qmlApp->setTemplateInfo(templateInfo);
}

void QmlApplicationWizard::createInstances(ExtensionSystem::IPlugin *plugin)
{
    foreach (const TemplateInfo &templateInfo, QmlApp::templateInfos()) {
        BaseFileWizardParameters parameters;
        parameters.setDisplayName(templateInfo.displayName);
        parameters.setDescription(templateInfo.description);
        parameters.setDescriptionImage(templateInfo.templatePath + QLatin1String("/template.png"));
        parameters.setCategory(
                    QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
        parameters.setDisplayCategory(
                    QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
        parameters.setKind(IWizard::ProjectWizard);
        parameters.setId(templateInfo.wizardId);

        QStringList stringList =
                templateInfo.featuresRequired.split(QLatin1Char(','), QString::SkipEmptyParts);;
        FeatureSet features;
        foreach (const QString &string, stringList) {
            Feature feature(Id::fromString(string.trimmed()));
            features |= feature;
        }

        parameters.setRequiredFeatures(features);
        parameters.setIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_QTQUICK_APP)));
        QmlApplicationWizard *wizard = new QmlApplicationWizard(parameters, templateInfo);
        plugin->addAutoReleasedObject(wizard);
    }
}

BaseFileWizardParameters QmlApplicationWizard::parameters()
{
    BaseFileWizardParameters params(ProjectWizard);
    params.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    params.setId(QLatin1String("QA.QMLB Application"));
    params.setIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_QTQUICK_APP)));
    params.setDisplayCategory(
                QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    params.setDisplayName(tr("Qt Quick Application"));
    params.setDescription(tr("Creates a Qt Quick application project."));
    return params;
}

QWizard *QmlApplicationWizard::createWizardDialog(QWidget *parent,
    const WizardDialogParameters &wizardDialogParameters) const
{
    QmlApplicationWizardDialog *wizardDialog = new QmlApplicationWizardDialog(m_qmlApp,
                                                                              parent, wizardDialogParameters);

    connect(wizardDialog, SIGNAL(projectParametersChanged(QString,QString)), m_qmlApp,
        SLOT(setProjectNameAndBaseDirectory(QString,QString)));

    wizardDialog->setPath(wizardDialogParameters.defaultPath());

    foreach (QWizardPage *page, wizardDialogParameters.extensionPages())
        applyExtensionPageShortTitle(wizardDialog, wizardDialog->addPage(page));

    return wizardDialog;
}

void QmlApplicationWizard::writeUserFile(const QString &fileName) const
{
    Manager *manager = ExtensionSystem::PluginManager::getObject<Manager>();

    QmlProject *project = new QmlProject(manager, fileName);
    QtSupport::QtVersionKitMatcher featureMatcher(requiredFeatures());
    QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::instance()->kits();
    foreach (ProjectExplorer::Kit *kit, kits)
        if (featureMatcher.matches(kit)
                && project->supportsKit(kit, 0)) // checks for desktop device
            project->addTarget(project->createTarget(kit));

    project->saveSettings();
    delete project;
}

GeneratedFiles QmlApplicationWizard::generateFiles(const QWizard * /*wizard*/,
                                                       QString *errorMessage) const
{
    return m_qmlApp->generateFiles(errorMessage);
}

bool QmlApplicationWizard::postGenerateFiles(const QWizard * /*wizard*/, const GeneratedFiles &l,
    QString *errorMessage)
{
    writeUserFile(m_qmlApp->creatorFileName());
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace QmlProjectManager
