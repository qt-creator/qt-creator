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

#include "testwizardpage.h"
#include "testwizarddialog.h"
#include "qtwizard.h"
#include "ui_testwizardpage.h"

namespace Qt4ProjectManager {
namespace Internal {

TestWizardPage::TestWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_sourceSuffix(QtWizard::sourceSuffix()),
    m_lowerCaseFileNames(QtWizard::lowerCaseFiles()),
    ui(new Ui::TestWizardPage),
    m_fileNameEdited(false),
    m_valid(false)
{
    setTitle(tr("Test Class Information"));
    ui->setupUi(this);
    ui->testSlotLineEdit->setText(QLatin1String("testCase1"));
    ui->testClassLineEdit->setLowerCaseFileName(m_lowerCaseFileNames);
    ui->qApplicationCheckBox->setChecked(TestWizardParameters::requiresQApplicationDefault);
    connect(ui->testClassLineEdit, SIGNAL(updateFileName(QString)),
            this, SLOT(slotClassNameEdited(QString)));
    connect(ui->fileLineEdit, SIGNAL(textEdited(QString)), \
                this, SLOT(slotFileNameEdited()));
    connect(ui->testClassLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotUpdateValid()));
    connect(ui->testSlotLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotUpdateValid()));
    connect(ui->fileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotUpdateValid()));
}

TestWizardPage::~TestWizardPage()
{
    delete ui;
}

bool TestWizardPage::isComplete() const
{
    return m_valid;
}

QString TestWizardPage::sourcefileName() const
{
    return ui->fileLineEdit->text();
}

TestWizardParameters TestWizardPage::parameters() const
{
    TestWizardParameters rc;
    rc.type = static_cast<TestWizardParameters::Type>(ui->typeComboBox->currentIndex());
    rc.initializationCode = ui->initCheckBox->isChecked();
    rc.useDataSet = ui->dataCheckBox->isChecked();
    rc.requiresQApplication = ui->qApplicationCheckBox->isChecked();
    rc.className = ui->testClassLineEdit->text();
    rc.testSlot = ui->testSlotLineEdit->text();
    rc.fileName = sourcefileName();
    return rc;
}

static QString fileNameFromClass(const QString &className,
                                 bool lowerCase,
                                 const QString &suffix)
{
    QString rc = QLatin1String(TestWizardParameters::filePrefix);
    rc += lowerCase ? className.toLower() : className;
    rc += QLatin1Char('.');
    rc += suffix;
    return rc;
}

void TestWizardPage::setProjectName(const QString &t)
{
    if (t.isEmpty())
        return;
    // initially populate
    QString className = t;
    className[0] = className.at(0).toUpper();
    className += QLatin1String("Test");
    ui->testClassLineEdit->setText(className);
    ui->fileLineEdit->setText(fileNameFromClass(className, m_lowerCaseFileNames, m_sourceSuffix));
}

void TestWizardPage::slotClassNameEdited(const QString &className)
{
    if (!m_fileNameEdited)
        ui->fileLineEdit->setText(fileNameFromClass(className, m_lowerCaseFileNames, m_sourceSuffix));
}

void TestWizardPage::slotFileNameEdited()
{
    m_fileNameEdited = true;
}

void TestWizardPage::slotUpdateValid()
{
    const bool newValid = ui->fileLineEdit->isValid()
                          && ui->testClassLineEdit->isValid()
                          && ui->testSlotLineEdit->isValid();
    if (newValid != m_valid) {
        m_valid = newValid;
        emit completeChanged();
    }
}

void TestWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
