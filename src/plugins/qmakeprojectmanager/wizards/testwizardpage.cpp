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

#include "testwizardpage.h"
#include "testwizarddialog.h"
#include "qtwizard.h"
#include "ui_testwizardpage.h"

#include <utils/wizard.h>

namespace QmakeProjectManager {
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
    connect(ui->testClassLineEdit, &Utils::ClassNameValidatingLineEdit::updateFileName,
            this, &TestWizardPage::slotClassNameEdited);
    connect(ui->fileLineEdit, &QLineEdit::textEdited, \
                this, &TestWizardPage::slotFileNameEdited);
    connect(ui->testClassLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &TestWizardPage::slotUpdateValid);
    connect(ui->testSlotLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &TestWizardPage::slotUpdateValid);
    connect(ui->fileLineEdit, &Utils::FancyLineEdit::validChanged,
            this, &TestWizardPage::slotUpdateValid);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Details"));
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

} // namespace Internal
} // namespace QmakeProjectManager
