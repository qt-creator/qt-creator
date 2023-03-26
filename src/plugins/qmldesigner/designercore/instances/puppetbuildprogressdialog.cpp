// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetbuildprogressdialog.h"
#include "ui_puppetbuildprogressdialog.h"

#include <QtDebug>
#include <QApplication>
#include <coreplugin/icore.h>

namespace  QmlDesigner {

PuppetBuildProgressDialog::PuppetBuildProgressDialog() :
    QDialog(Core::ICore::dialogParent()),
    ui(new Ui::PuppetBuildProgressDialog),
    m_lineCount(0),
    m_useFallbackPuppet(false)
{
    setWindowFlags(Qt::SplashScreen);
    setWindowModality(Qt::ApplicationModal);
    ui->setupUi(this);
    ui->buildProgressBar->setMaximum(85);
    connect(ui->useFallbackPuppetPushButton, &QAbstractButton::clicked, this, &PuppetBuildProgressDialog::setUseFallbackPuppet);
}

PuppetBuildProgressDialog::~PuppetBuildProgressDialog()
{
    delete ui;
}

void PuppetBuildProgressDialog::setProgress(int progress)
{
    ui->buildProgressBar->setValue(progress);
}

bool PuppetBuildProgressDialog::useFallbackPuppet() const
{
    return m_useFallbackPuppet;
}

void PuppetBuildProgressDialog::setErrorOutputFile(const QString &filePath)
{
    ui->openErrorOutputFileLabel->setText(QString::fromLatin1("<a href='file:///%1'>%2</a>").arg(
        filePath, ui->openErrorOutputFileLabel->text()));
}

void PuppetBuildProgressDialog::setErrorMessage(const QString &message)
{
    ui->label->setText(QString::fromLatin1("<font color='red'>%1</font>").arg(message));
    ui->useFallbackPuppetPushButton->setText(tr("OK"));
    connect(ui->useFallbackPuppetPushButton, &QAbstractButton::clicked, this, &QDialog::accept);
}

void PuppetBuildProgressDialog::setUseFallbackPuppet()
{
    m_useFallbackPuppet = true;
}

void PuppetBuildProgressDialog::newBuildOutput(const QByteArray &standardOutput)
{
    m_lineCount += standardOutput.count('\n');
    setProgress(m_lineCount);
}

}
