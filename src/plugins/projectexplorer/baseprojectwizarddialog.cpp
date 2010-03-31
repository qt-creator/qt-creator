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

#include "baseprojectwizarddialog.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <utils/projectintropage.h>

#include <QtCore/QDir>

namespace ProjectExplorer {

struct BaseProjectWizardDialogPrivate {
    explicit BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id = -1);

    const int desiredIntroPageId;
    Utils::ProjectIntroPage *introPage;
    int introPageId;
    int lastId;
};

BaseProjectWizardDialogPrivate::BaseProjectWizardDialogPrivate(Utils::ProjectIntroPage *page, int id) :
    desiredIntroPageId(id),
    introPage(page),
    introPageId(-1),
    lastId(-1)
{
}

BaseProjectWizardDialog::BaseProjectWizardDialog(QWidget *parent) :
    Utils::Wizard(parent),
    d(new BaseProjectWizardDialogPrivate(new Utils::ProjectIntroPage))
{
    init();
}

BaseProjectWizardDialog::BaseProjectWizardDialog(Utils::ProjectIntroPage *introPage,
                                                 int introId,
                                                 QWidget *parent) :
    Utils::Wizard(parent),
    d(new BaseProjectWizardDialogPrivate(introPage, introId))
{
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
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotBaseCurrentIdChanged(int)));
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

void BaseProjectWizardDialog::slotAccepted()
{
    if (d->introPage->useAsDefaultPath()) {
        // Store the path as default path for new projects if desired.
        Core::FileManager *fm = Core::ICore::instance()->fileManager();
        fm->setProjectsDirectory(path());
        fm->setUseProjectsDirectory(true);
    }
}

void BaseProjectWizardDialog::slotBaseCurrentIdChanged(int id)
{
    if (d->lastId == d->introPageId) {
        emit introPageLeft(d->introPage->projectName(), d->introPage->path());
    }
    d->lastId = id;
}

Utils::ProjectIntroPage *BaseProjectWizardDialog::introPage() const
{
    return d->introPage;
}

QString BaseProjectWizardDialog::uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks.
    const QString prefix = tr("untitled");
    for (unsigned i = 0; ; i++) {
        QString name = prefix;
        if (i)
            name += QString::number(i);
        if (!pathDir.exists(name))
            return name;
    }
    return prefix;
}
} // namespace ProjectExplorer
