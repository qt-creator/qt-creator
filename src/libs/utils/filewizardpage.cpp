/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "filewizardpage.h"
#include "ui_filewizardpage.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QMessageBox>

namespace Core {
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
    switch(e->type()) {
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
    return FileNameValidatingLineEdit::validateFileName(name, errorMessage);
}

} // namespace Utils
} // namespace Core
