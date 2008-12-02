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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "projectwizardpage.h"
#include "ui_projectwizardpage.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;
using namespace Internal;

ProjectWizardPage::ProjectWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::WizardPage)
{
    m_ui->setupUi(this);

    connect(m_ui->addToProjectCheckBox, SIGNAL(toggled(bool)),
            m_ui->projectComboBox, SLOT(setEnabled(bool)));
}

ProjectWizardPage::~ProjectWizardPage()
{
    delete m_ui;
}

void ProjectWizardPage::setProjects(const QStringList &l)
{
    m_ui->projectComboBox->clear();
    m_ui->projectComboBox->addItems(l);
}

void ProjectWizardPage::setAddToProjectEnabled(bool b)
{
    m_ui->projectLabel->setEnabled(b);
    m_ui->addToProjectLabel->setEnabled(b);
    m_ui->addToProjectCheckBox->setChecked(b);
    m_ui->addToProjectCheckBox->setEnabled(b);
    m_ui->projectComboBox->setEnabled(b);
}

int ProjectWizardPage::currentProjectIndex() const
{
    return m_ui->projectComboBox->currentIndex();
}

void ProjectWizardPage::setCurrentProjectIndex(int i)
{
    m_ui->projectComboBox->setCurrentIndex(i);
}

bool ProjectWizardPage::addToProject() const
{
    return m_ui->addToProjectCheckBox->isChecked();
}

bool ProjectWizardPage::addToVersionControl() const
{
    return m_ui->addToVersionControlCheckBox->isChecked();
}

void ProjectWizardPage::setAddToVersionControlEnabled(bool b)
{
    m_ui->addToVersionControlLabel->setEnabled(b);
    m_ui->addToVersionControlCheckBox->setChecked(b);
    m_ui->addToVersionControlCheckBox->setEnabled(b);
}

void ProjectWizardPage::changeEvent(QEvent *e)
{
    switch(e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ProjectWizardPage::setFilesDisplay(const QStringList &files)
{
    QString fileMessage; {
        QTextStream str(&fileMessage);
        str << "<html>Files to be added:<pre>";
        const QStringList::const_iterator cend = files.constEnd();
        for (QStringList::const_iterator it = files.constBegin(); it != cend; ++it)
            str << *it << '\n';
        str << "</pre>";
    }
    m_ui->filesLabel->setText(fileMessage);
}
