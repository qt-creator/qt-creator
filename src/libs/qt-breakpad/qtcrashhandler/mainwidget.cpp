/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPair>
#include <QStringList>
#include <QTemporaryFile>
#include <QUrl>

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    connect(ui->restartButton, &QAbstractButton::clicked, this, &MainWidget::restartApplication);
    connect(ui->quitButton, &QAbstractButton::clicked, this, &MainWidget::quitApplication);
    connect(ui->detailButton, &QAbstractButton::clicked, this, &MainWidget::showDetails);
    connect(ui->commentTextEdit, &QTextEdit::textChanged, this, &MainWidget::commentIsProvided);
    connect(ui->emailLineEdit, &QLineEdit::textEdited, this, &MainWidget::emailAdressChanged);
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::setProgressbarMaximum(int maximum)
{
    ui->progressBar->setMaximum(maximum);
}

void MainWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
}

void MainWidget::updateProgressBar(qint64 progressCount, qint64 fullCount)
{
    ui->progressBar->setValue(static_cast<int>(progressCount));
    ui->progressBar->setMaximum(static_cast<int>(fullCount));
}

void MainWidget::showError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (error != QNetworkReply::NoError && reply) {
        ui->commentTextEdit->setReadOnly(true);
        ui->commentTextEdit->setPlainText(reply->errorString());
    }
}

void MainWidget::restartApplication()
{
    if (ui->sendDumpCheckBox->isChecked())
        emit restartCrashedApplicationAndSendDump();
    else
        emit restartCrashedApplication();
}

void MainWidget::quitApplication()
{
    ui->quitButton->setEnabled(false);
    if (ui->sendDumpCheckBox->isChecked())
        emit sendDump();
    else
        QCoreApplication::quit();
}

void MainWidget::commentIsProvided()
{
    m_commentIsProvided = true;
    emit commentChanged(ui->commentTextEdit->toPlainText());
}

void MainWidget::showDetails()
{
    if (m_detailDialog.isNull()) {
        m_detailDialog = new DetailDialog(this);

        QString detailText;

        detailText.append(tr("We specifically send the following information:\n\n"));

        QString dumpPath = QApplication::arguments().at(1);
        QString startupTime = QApplication::arguments().at(2);
        QString applicationName = QApplication::arguments().at(3);
        QString applicationVersion = QApplication::arguments().at(4);
        QString plugins = QApplication::arguments().at(5);
        QString ideRevision = QApplication::arguments().at(6);

        detailText.append(QString("StartupTime: %1\n").arg(startupTime));
        detailText.append(QString("Vendor: %1\n").arg("Qt Project"));
        detailText.append(QString("InstallTime: %1\n").arg("0"));
        detailText.append(QString("Add-ons: %1\n").arg(plugins));
        detailText.append(QString("BuildID: %1\n").arg("0"));
        detailText.append(QString("SecondsSinceLastCrash: %1\n").arg("0"));
        detailText.append(QString("ProductName: %1\n").arg(applicationName));
        detailText.append(QString("URL: %1\n").arg(""));
        detailText.append(QString("Theme: %1\n").arg(""));
        detailText.append(QString("Version: %1\n").arg(applicationVersion));
        detailText.append(QString("CrashTime: %1\n").arg(QString::number(QDateTime::currentDateTime().toTime_t())));

        if (!ui->emailLineEdit->text().isEmpty())
            detailText.append(tr("Email: %1\n").arg(ui->emailLineEdit->text()));

        if (m_commentIsProvided)
            detailText.append(tr("Comments: %1\n").arg(ui->commentTextEdit->toPlainText()));

        detailText.append(
                    tr("In addition, we send a Microsoft Minidump file, which contains information "
                       "about this computer, such as the operating system and CPU, and most "
                       "importantly, it contains the stacktrace, which is an internal structure that "
                       "shows where the program crashed. This information will help us to identify "
                       "the cause of the crash and to fix it."));

        m_detailDialog.data()->setText(detailText);
    }
    if (m_detailDialog->isVisible())
        m_detailDialog->showNormal();
    else
        m_detailDialog->show();
}
