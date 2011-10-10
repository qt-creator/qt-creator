/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "newtestcasedlg.h"
#include "ui_newtestcasedlg.h"
#include "testgenerator.h"

#include <QFileDialog>
#include <QRegExpValidator>
#include <QRegExp>
#include <QDebug>

NewTestCaseDlg::NewTestCaseDlg(const QString &path, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTestCaseDlg)
{
    ui->setupUi(this);
    m_pathIsEdited = false;
    ui->location->setText(path + "/tests");

    connect(ui->rbUnit, SIGNAL(clicked(bool)), this, SLOT(onChanged()));
    connect(ui->rbIntegration, SIGNAL(clicked(bool)), this, SLOT(onChanged()));
    connect(ui->rbPerformance, SIGNAL(clicked(bool)), this, SLOT(onChanged()));
    connect(ui->rbSystem, SIGNAL(clicked(bool)), this, SLOT(onChanged()));
    connect(ui->testName, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(ui->componentName, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(ui->location, SIGNAL(textChanged(QString)), this, SLOT(onLocationChanged(QString)));
    connect(ui->locationBtn, SIGNAL(clicked()), this, SLOT(onLocationBtnClicked()));
    connect(ui->prependComponentName, SIGNAL(stateChanged(int)),
        this, SLOT(onComponentChkBoxChanged(int)));

    m_originalPalette = palette();
    ui->rbUnit->setChecked(true);

    // set a validator for test case name
    QRegExp testCaseRegExp(QLatin1String("([a-z]{3,3})_([a-z]|[A-Z])([a-z]|[A-Z]|\\d|_|-)+"),
        Qt::CaseSensitive);
    QRegExpValidator *testCaseValidator = new QRegExpValidator(testCaseRegExp, this);
    ui->testName->setValidator(testCaseValidator);

    onTextChanged("");
}

NewTestCaseDlg::~NewTestCaseDlg()
{
    delete ui;
}

void NewTestCaseDlg::onTextChanged(const QString &txt)
{
    Q_UNUSED(txt);
    onChanged();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->testName->text().length() > 4);
}

void NewTestCaseDlg::onLocationChanged(const QString &txt)
{
    Q_UNUSED(txt);
    m_pathIsEdited = true;
    onChanged();
}

void NewTestCaseDlg::onChanged()
{
    QString txt = ui->testName->text();
    QString prefix;
    QString subdir;

    switch (mode()) {
    case TestGenerator::UnitTest:
        prefix = "tst_";
        subdir = "auto";
        break;
    case TestGenerator::PerformanceTest:
        prefix = "prf_";
        subdir = "benchmarks";
        break;
    case TestGenerator::IntegrationTest:
        prefix = "int_";
        subdir = "auto";
        break;
    case TestGenerator::SystemTest:
        prefix = "sys_";
        subdir = "systemtests";
        break;
    }

    if (!txt.startsWith(prefix)) {
        QString rest = txt.mid(4);
        ui->testName->setText(prefix + rest);
    }

    if (QFile::exists(ui->location->text())) {
        ui->location->setPalette(m_originalPalette);
    } else {
        // use a highlighted text color if directory does not exist
        QPalette pal = ui->location->palette();
        pal.setColor(QPalette::Text, Qt::red); // TODO make use of a theme friendly color
        ui->location->setPalette(pal);
    }

    // show the user the files that would be created
    ui->filesList->clear();

    m_testCaseGenerator.setTestCase(mode(), location(), "", testCaseName(),
        testedComponent(), testedClassName(), "");

    ui->filesList->addItem(m_testCaseGenerator.generatedProName());
    ui->filesList->addItem(m_testCaseGenerator.generatedFileName());
}

QString NewTestCaseDlg::actualPath()
{
    return m_testCaseGenerator.testCaseDirectory();
}

TestGenerator::GenMode NewTestCaseDlg::mode()
{
    if (ui->rbUnit->isChecked())
        return TestGenerator::UnitTest;
    else if (ui->rbIntegration->isChecked())
        return TestGenerator::IntegrationTest;
    else if (ui->rbPerformance->isChecked())
        return TestGenerator::PerformanceTest;
    else
        return TestGenerator::SystemTest;
}

QString NewTestCaseDlg::testCaseName()
{
    return ui->testName->text();
}

QString NewTestCaseDlg::location()
{
    return ui->location->text();
}

QString NewTestCaseDlg::testedClassName()
{
    return ui->testedClass->text();
}

QString NewTestCaseDlg::testedComponent()
{
    return ui->componentName->text();
}

void NewTestCaseDlg::onLocationBtnClicked()
{
    QString absoluteTestPath =
        QFileDialog::getExistingDirectory(this, tr("Select Absolute Test Case Directory"),
        ui->location->text());
    if (!absoluteTestPath.isEmpty()) {
        ui->location->setText(absoluteTestPath);
        onChanged();
    }
}

void NewTestCaseDlg::onComponentChkBoxChanged(int state)
{
   m_testCaseGenerator.enableComponentInTestName(state == Qt::Checked);
   onChanged();
}


bool NewTestCaseDlg::componentInName()
{
    return (ui->prependComponentName->checkState() == Qt::Checked);
}
