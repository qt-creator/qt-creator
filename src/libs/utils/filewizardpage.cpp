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

#include "filewizardpage.h"
#include "ui_filewizardpage.h"

namespace Utils {

struct FileWizardPagePrivate
{
    FileWizardPagePrivate();
    Ui::WizardPage m_ui;
    bool m_complete;
};

FileWizardPagePrivate::FileWizardPagePrivate() :
    m_complete(false)
{
}

FileWizardPage::FileWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_d(new FileWizardPagePrivate)
{
    m_d->m_ui.setupUi(this);
    connect(m_d->m_ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));

    connect(m_d->m_ui.pathChooser, SIGNAL(returnPressed()), this, SLOT(slotActivated()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(validReturnPressed()), this, SLOT(slotActivated()));
}

FileWizardPage::~FileWizardPage()
{
    delete m_d;
}

QString FileWizardPage::name() const
{
    return m_d->m_ui.nameLineEdit->text();
}

QString FileWizardPage::path() const
{
    return m_d->m_ui.pathChooser->path();
}

void FileWizardPage::setPath(const QString &path)
{
    m_d->m_ui.pathChooser->setPath(path);
}

void FileWizardPage::setName(const QString &name)
{
    m_d->m_ui.nameLineEdit->setText(name);
}

void FileWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

bool FileWizardPage::isComplete() const
{
    return m_d->m_complete;
}

void FileWizardPage::setNameLabel(const QString &label)
{
    m_d->m_ui.nameLabel->setText(label);
}

void FileWizardPage::setPathLabel(const QString &label)
{
    m_d->m_ui.pathLabel->setText(label);
}

void FileWizardPage::slotValidChanged()
{
    const bool newComplete = m_d->m_ui.pathChooser->isValid() && m_d->m_ui.nameLineEdit->isValid();
    if (newComplete != m_d->m_complete) {
        m_d->m_complete = newComplete;
        emit completeChanged();
    }
}

void FileWizardPage::slotActivated()
{
    if (m_d->m_complete)
        emit activated();
}

bool FileWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}

} // namespace Utils
