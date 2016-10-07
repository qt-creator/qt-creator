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

#include "puppetbuildprogressdialog.h"
#include "ui_puppetbuildprogressdialog.h"

#include <QProcess>
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
    connect(ui->useFallbackPuppetPushButton, SIGNAL(clicked()), this, SLOT(setUseFallbackPuppet()));
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
    connect(ui->useFallbackPuppetPushButton, SIGNAL(clicked()), this, SLOT(accept()));
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
