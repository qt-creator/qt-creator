/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basecheckoutwizardpage.h"
#include "ui_basecheckoutwizardpage.h"

namespace VCSBase {

struct BaseCheckoutWizardPagePrivate {
    BaseCheckoutWizardPagePrivate() : m_valid(false), m_directoryEdited(false) {}

    Ui::BaseCheckoutWizardPage ui;
    bool m_valid;
    bool m_directoryEdited;
};

BaseCheckoutWizardPage::BaseCheckoutWizardPage(QWidget *parent) :
    QWizardPage(parent),
    d(new BaseCheckoutWizardPagePrivate)
{
    d->ui.setupUi(this);
    d->ui.pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    connect(d->ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotChanged()));
    connect(d->ui.checkoutDirectoryLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotChanged()));
    connect(d->ui.repositoryLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotRepositoryChanged(QString)));
    connect(d->ui.checkoutDirectoryLineEdit, SIGNAL(textEdited(QString)), this, SLOT(slotDirectoryEdited()));
}

BaseCheckoutWizardPage::~BaseCheckoutWizardPage()
{
    delete d;
}

void BaseCheckoutWizardPage::setRepositoryLabel(const QString &l)
{
    d->ui.repositoryLabel->setText(l);
}

bool BaseCheckoutWizardPage::isRepositoryReadOnly() const
{
    return d->ui.repositoryLineEdit->isReadOnly();
}

void BaseCheckoutWizardPage::setRepositoryReadOnly(bool v)
{
    d->ui.repositoryLineEdit->setReadOnly(v);
}

QString BaseCheckoutWizardPage::path() const
{
    return d->ui.pathChooser->path();
}

void BaseCheckoutWizardPage::setPath(const QString &p)
{
    d->ui.pathChooser->setPath(p);
}

QString BaseCheckoutWizardPage::directory() const
{
    return d->ui.checkoutDirectoryLineEdit->text();
}

void BaseCheckoutWizardPage::setDirectory(const QString &dir)
{
    d->ui.checkoutDirectoryLineEdit->setText(dir);
}

void BaseCheckoutWizardPage::setDirectoryVisible(bool v)
{
    d->ui.checkoutDirectoryLabel->setVisible(v);
    d->ui.checkoutDirectoryLineEdit->setVisible(v);
}

QString BaseCheckoutWizardPage::repository() const
{
    return d->ui.repositoryLineEdit->text().trimmed();
}

void BaseCheckoutWizardPage::setRepository(const QString &r)
{
    d->ui.repositoryLineEdit->setText(r);
}

void BaseCheckoutWizardPage::slotRepositoryChanged(const QString &repo)
{
    if (d->m_directoryEdited)
        return;
    d->ui.checkoutDirectoryLineEdit->setText(directoryFromRepository(repo));
}

QString BaseCheckoutWizardPage::directoryFromRepository(const QString &r) const
{
    return r;
}

void BaseCheckoutWizardPage::slotDirectoryEdited()
{
    d->m_directoryEdited = true;
}

void BaseCheckoutWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d->ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

bool BaseCheckoutWizardPage::isComplete() const
{
    return d->m_valid;
}

void BaseCheckoutWizardPage::slotChanged()
{
    const bool valid = d->ui.pathChooser->isValid()
                       && d->ui.checkoutDirectoryLineEdit->isValid()
                       && !d->ui.repositoryLineEdit->text().isEmpty();

    if (valid != d->m_valid) {
        d->m_valid = valid;
        emit completeChanged();
    }
}

} // namespace VCSBase
