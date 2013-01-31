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

#include "baseprojectwizarddialog.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <utils/projectintropage.h>

#include <QDir>

/*!
    \class ProjectExplorer::BaseProjectWizardDialog

    \brief Base class for project wizards.

    Presents the introductory page and takes care of setting the folder chosen
    as default projects' folder should the user wish to do that.
*/

namespace ProjectExplorer {

struct BaseProjectWizardDialogPrivate {
    explicit BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id = -1);

    const int desiredIntroPageId;
    Utils::ProjectIntroPage *introPage;
    int introPageId;
    QString selectedPlatform;
    Core::FeatureSet requiredFeatureSet;
};

BaseProjectWizardDialogPrivate::BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id) :
    desiredIntroPageId(id),
    introPage(page),
    introPageId(-1)
{
}

BaseProjectWizardDialog::BaseProjectWizardDialog(QWidget *parent,
                                                 const Core::WizardDialogParameters &parameters) :
    Utils::Wizard(parent),
    d(new BaseProjectWizardDialogPrivate(new Utils::ProjectIntroPage))
{
    setPath(parameters.defaultPath());
    setSelectedPlatform(parameters.selectedPlatform());
    setRequiredFeatures(parameters.requiredFeatures());
    init();
}

BaseProjectWizardDialog::BaseProjectWizardDialog(Utils::ProjectIntroPage *introPage,
                                                 int introId,
                                                 QWidget *parent,
                                                 const Core::WizardDialogParameters &parameters) :
    Utils::Wizard(parent),
    d(new BaseProjectWizardDialogPrivate(introPage, introId))
{
    setPath(parameters.defaultPath());
    setSelectedPlatform(parameters.selectedPlatform());
    setRequiredFeatures(parameters.requiredFeatures());
    init();
}

void BaseProjectWizardDialog::init()
{
    Core::BaseFileWizard::setupWizard(this);
    if (d->introPageId == -1) {
        d->introPageId = addPage(d->introPage);
    } else {
        d->introPageId = d->desiredIntroPageId;
        setPage(d->desiredIntroPageId, d->introPage);
    }
    wizardProgress()->item(d->introPageId)->setTitle(tr("Location"));
    connect(this, SIGNAL(accepted()), this, SLOT(slotAccepted()));
    connect(this, SIGNAL(nextClicked()), this, SLOT(nextClicked()));
}

BaseProjectWizardDialog::~BaseProjectWizardDialog()
{
    delete d;
}

QString BaseProjectWizardDialog::projectName() const
{
    return d->introPage->projectName();
}

QString BaseProjectWizardDialog::path() const
{
    return d->introPage->path();
}

void BaseProjectWizardDialog::setIntroDescription(const QString &des)
{
    d->introPage->setDescription(des);
}

void BaseProjectWizardDialog::setPath(const QString &path)
{
    d->introPage->setPath(path);
}

void BaseProjectWizardDialog::setProjectName(const QString &name)
{
    d->introPage->setProjectName(name);
}

void BaseProjectWizardDialog::setProjectList(const QStringList &projectList)
{
    d->introPage->setProjectList(projectList);
}

void BaseProjectWizardDialog::setProjectDirectories(const QStringList &directories)
{
    d->introPage->setProjectDirectories(directories);
}

void BaseProjectWizardDialog::setForceSubProject(bool force)
{
    introPage()->setForceSubProject(force);
}

void BaseProjectWizardDialog::slotAccepted()
{
    if (d->introPage->useAsDefaultPath()) {
        // Store the path as default path for new projects if desired.
        Core::DocumentManager::setProjectsDirectory(path());
        Core::DocumentManager::setUseProjectsDirectory(true);
    }
}

void BaseProjectWizardDialog::nextClicked()
{
    if (currentId() == d->introPageId)
        emit projectParametersChanged(d->introPage->projectName(), d->introPage->path());
}

Utils::ProjectIntroPage *BaseProjectWizardDialog::introPage() const
{
    return d->introPage;
}

QString BaseProjectWizardDialog::uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks
    //: and using only ascii chars.
    const QString prefix = tr("untitled");
    for (unsigned i = 0; ; ++i) {
        QString name = prefix;
        if (i)
            name += QString::number(i);
        if (!pathDir.exists(name))
            return name;
    }
    return prefix;
}

void BaseProjectWizardDialog::addExtensionPages(const QList<QWizardPage *> &wizardPageList)
{
    foreach (QWizardPage *p,wizardPageList)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
}

QString BaseProjectWizardDialog::selectedPlatform() const
{
    return d->selectedPlatform;
}

void BaseProjectWizardDialog::setSelectedPlatform(const QString &platform)
{
    d->selectedPlatform = platform;
}

Core::FeatureSet BaseProjectWizardDialog::requiredFeatures() const
{
    return d->requiredFeatureSet;
}

void BaseProjectWizardDialog::setRequiredFeatures(const Core::FeatureSet &featureSet)
{
    d->requiredFeatureSet = featureSet;
}

} // namespace ProjectExplorer
