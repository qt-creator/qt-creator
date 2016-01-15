/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baseprojectwizarddialog.h"

#include <coreplugin/documentmanager.h>
#include <utils/projectintropage.h>

#include <QDir>

/*!
    \class ProjectExplorer::BaseProjectWizardDialog

    \brief The BaseProjectWizardDialog class is the base class for project
    wizards.

    Presents the introductory page and takes care of setting the folder chosen
    as default projects' folder should the user wish to do that.
*/

namespace ProjectExplorer {

struct BaseProjectWizardDialogPrivate {
    explicit BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id = -1);

    const int desiredIntroPageId;
    Utils::ProjectIntroPage *introPage;
    int introPageId;
    Core::Id selectedPlatform;
    QSet<Core::Id> requiredFeatureSet;
};

BaseProjectWizardDialogPrivate::BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id) :
    desiredIntroPageId(id),
    introPage(page),
    introPageId(-1)
{
}

BaseProjectWizardDialog::BaseProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                 QWidget *parent,
                                                 const Core::WizardDialogParameters &parameters) :
    Core::BaseFileWizard(factory, parameters.extraValues(), parent),
    d(new BaseProjectWizardDialogPrivate(new Utils::ProjectIntroPage))
{
    setPath(parameters.defaultPath());
    setSelectedPlatform(parameters.selectedPlatform());
    setRequiredFeatures(parameters.requiredFeatures());
    init();
}

BaseProjectWizardDialog::BaseProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                 Utils::ProjectIntroPage *introPage, int introId,
                                                 QWidget *parent,
                                                 const Core::WizardDialogParameters &parameters) :
    Core::BaseFileWizard(factory, parameters.extraValues(), parent),
    d(new BaseProjectWizardDialogPrivate(introPage, introId))
{
    setPath(parameters.defaultPath());
    setSelectedPlatform(parameters.selectedPlatform());
    setRequiredFeatures(parameters.requiredFeatures());
    init();
}

void BaseProjectWizardDialog::init()
{
    if (d->introPageId == -1) {
        d->introPageId = addPage(d->introPage);
    } else {
        d->introPageId = d->desiredIntroPageId;
        setPage(d->desiredIntroPageId, d->introPage);
    }
    connect(this, &QDialog::accepted, this, &BaseProjectWizardDialog::slotAccepted);
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

bool BaseProjectWizardDialog::validateCurrentPage()
{
    if (currentId() == d->introPageId)
        emit projectParametersChanged(d->introPage->projectName(), d->introPage->path());
    return Core::BaseFileWizard::validateCurrentPage();
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
        addPage(p);
}

Core::Id BaseProjectWizardDialog::selectedPlatform() const
{
    return d->selectedPlatform;
}

void BaseProjectWizardDialog::setSelectedPlatform(Core::Id platform)
{
    d->selectedPlatform = platform;
}

QSet<Core::Id> BaseProjectWizardDialog::requiredFeatures() const
{
    return d->requiredFeatureSet;
}

void BaseProjectWizardDialog::setRequiredFeatures(const QSet<Core::Id> &featureSet)
{
    d->requiredFeatureSet = featureSet;
}

} // namespace ProjectExplorer
