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

#include "basecheckoutwizardpage.h"
#include "ui_basecheckoutwizardpage.h"

#include <QIcon>

/*!
    \class VcsBase::BaseCheckoutWizardPage

    \brief Base class for a parameter page of a checkout wizard.

    Lets the user specify the repository, a checkout directory and
    the path. Contains a virtual to derive the checkout directory
    from the repository as it is entered.

    \sa VcsBase::BaseCheckoutWizard
*/

namespace VcsBase {
namespace Internal {

class BaseCheckoutWizardPagePrivate
{
public:
    BaseCheckoutWizardPagePrivate() : m_valid(false), m_directoryEdited(false) {}

    Internal::Ui::BaseCheckoutWizardPage ui;
    bool m_valid;
    bool m_directoryEdited;
};

} // namespace Internal

BaseCheckoutWizardPage::BaseCheckoutWizardPage(QWidget *parent) :
    QWizardPage(parent),
    d(new Internal::BaseCheckoutWizardPagePrivate)
{
    d->ui.setupUi(this);

    connect(d->ui.repositoryLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotRepositoryChanged(QString)));

    connect(d->ui.checkoutDirectoryLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotChanged()));
    connect(d->ui.checkoutDirectoryLineEdit, SIGNAL(textEdited(QString)), this, SLOT(slotDirectoryEdited()));
    connect(d->ui.branchComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotChanged()));

    d->ui.pathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    connect(d->ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotChanged()));

    d->ui.branchComboBox->setEnabled(false);
    d->ui.branchRefreshToolButton->setIcon(QIcon(QLatin1String(":/locator/images/reload.png")));
    connect(d->ui.branchRefreshToolButton, SIGNAL(clicked()), this, SLOT(slotRefreshBranches()));
}

BaseCheckoutWizardPage::~BaseCheckoutWizardPage()
{
    delete d;
}

void BaseCheckoutWizardPage::addLocalControl(QWidget *w)
{
    d->ui.localLayout->addRow(w);
}

void BaseCheckoutWizardPage::addLocalControl(QString &description, QWidget *w)
{
    d->ui.localLayout->addRow(description, w);
}

void BaseCheckoutWizardPage::addRepositoryControl(QWidget *w)
{
    d->ui.repositoryLayout->addRow(w);
}

bool BaseCheckoutWizardPage::checkIsValid() const
{
    return d->ui.pathChooser->isValid()
            && !d->ui.checkoutDirectoryLineEdit->text().isEmpty()
            && !d->ui.repositoryLineEdit->text().isEmpty();
}

void BaseCheckoutWizardPage::addRepositoryControl(QString &description, QWidget *w)
{
    d->ui.repositoryLayout->addRow(description, w);
}

bool BaseCheckoutWizardPage::isBranchSelectorVisible() const
{
    return d->ui.branchComboBox->isVisible();
}

void BaseCheckoutWizardPage::setBranchSelectorVisible(bool v)
{
    d->ui.branchComboBox->setVisible(v);
    d->ui.branchRefreshToolButton->setVisible(v);
    d->ui.branchLabel->setVisible(v);
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

QString BaseCheckoutWizardPage::branch() const
{
    return d->ui.branchComboBox->currentText();
}

void BaseCheckoutWizardPage::setBranch(const QString &b)
{
    const int index = d->ui.branchComboBox->findText(b);
    if (index != -1)
        d->ui.branchComboBox->setCurrentIndex(index);
}

void BaseCheckoutWizardPage::slotRefreshBranches()
{
    if (!isBranchSelectorVisible())
        return;
    // Refresh branch list on demand. This is hard to make
    // automagically since there can be network slowness/timeouts, etc.
    int current;
    const QStringList branchList = branches(repository(), &current);
    d->ui.branchComboBox->clear();
    d->ui.branchComboBox->setEnabled(branchList.size() > 1);
    if (!branchList.isEmpty()) {
        d->ui.branchComboBox->addItems(branchList);
        if (current >= 0 && current < branchList.size())
            d->ui.branchComboBox->setCurrentIndex(current);
    }
    slotChanged();
}

void BaseCheckoutWizardPage::slotRepositoryChanged(const QString &repo)
{
    // Derive directory name from repository unless user manually edited it.
    if (!d->m_directoryEdited)
        d->ui.checkoutDirectoryLineEdit->setText(directoryFromRepository(repo));
    slotChanged();
}

QString BaseCheckoutWizardPage::directoryFromRepository(const QString &r) const
{
    return r;
}

QStringList BaseCheckoutWizardPage::branches(const QString &, int *)
{
    return QStringList();
}

void BaseCheckoutWizardPage::slotDirectoryEdited()
{
    d->m_directoryEdited = true;
    slotChanged();
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
    const bool valid = checkIsValid();

    if (valid != d->m_valid) {
        d->m_valid = valid;
        emit completeChanged();
    }
}

} // namespace VcsBase
