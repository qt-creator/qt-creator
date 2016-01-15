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

#include "filewizardpage.h"
#include "ui_filewizardpage.h"

#include "wizard.h"

/*!
  \class Utils::FileWizardPage

  \brief The FileWizardPage class is a standard wizard page for a single file
  letting the user choose name
  and path.

  The name and path labels can be changed. By default they are simply "Name:"
  and "Path:".
*/

namespace Utils {

class FileWizardPagePrivate
{
public:
    FileWizardPagePrivate();
    Ui::WizardPage m_ui;
    bool m_complete;
};

FileWizardPagePrivate::FileWizardPagePrivate() :
    m_complete(false)
{
}

FileWizardPage::FileWizardPage(QWidget *parent) :
    WizardPage(parent),
    d(new FileWizardPagePrivate)
{
    d->m_ui.setupUi(this);
    connect(d->m_ui.pathChooser, &PathChooser::validChanged,
            this, &FileWizardPage::slotValidChanged);
    connect(d->m_ui.nameLineEdit, &FancyLineEdit::validChanged,
            this, &FileWizardPage::slotValidChanged);

    connect(d->m_ui.pathChooser, &PathChooser::returnPressed,
            this, &FileWizardPage::slotActivated);
    connect(d->m_ui.nameLineEdit, &FancyLineEdit::validReturnPressed,
            this, &FileWizardPage::slotActivated);

    setProperty(SHORT_TITLE_PROPERTY, tr("Location"));

    registerFieldWithName(QLatin1String("Path"), d->m_ui.pathChooser, "path", SIGNAL(pathChanged(QString)));
    registerFieldWithName(QLatin1String("FileName"), d->m_ui.nameLineEdit);
}

FileWizardPage::~FileWizardPage()
{
    delete d;
}

QString FileWizardPage::fileName() const
{
    return d->m_ui.nameLineEdit->text();
}

QString FileWizardPage::path() const
{
    return d->m_ui.pathChooser->path();
}

void FileWizardPage::setPath(const QString &path)
{
    d->m_ui.pathChooser->setPath(path);
}

void FileWizardPage::setFileName(const QString &name)
{
    d->m_ui.nameLineEdit->setText(name);
}

bool FileWizardPage::isComplete() const
{
    return d->m_complete;
}

void FileWizardPage::setFileNameLabel(const QString &label)
{
    d->m_ui.nameLabel->setText(label);
}

void FileWizardPage::setPathLabel(const QString &label)
{
    d->m_ui.pathLabel->setText(label);
}

bool FileWizardPage::forceFirstCapitalLetterForFileName() const
{
    return d->m_ui.nameLineEdit->forceFirstCapitalLetter();
}

void FileWizardPage::setForceFirstCapitalLetterForFileName(bool b)
{
    d->m_ui.nameLineEdit->setForceFirstCapitalLetter(b);
}

void FileWizardPage::slotValidChanged()
{
    const bool newComplete = d->m_ui.pathChooser->isValid() && d->m_ui.nameLineEdit->isValid();
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void FileWizardPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

bool FileWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}

} // namespace Utils
