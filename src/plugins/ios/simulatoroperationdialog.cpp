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

#include "simulatoroperationdialog.h"
#include "ui_simulatoroperationdialog.h"

#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QPushButton>

namespace {
Q_LOGGING_CATEGORY(iosCommon, "qtc.ios.common")
}

namespace Ios {
namespace Internal {

SimulatorOperationDialog::SimulatorOperationDialog(QWidget *parent) :
    // TODO: Maximize buttong only because of QTBUG-41932
    QDialog(parent,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint),
    m_ui(new Ui::SimulatorOperationDialog)
{
    m_ui->setupUi(this);

    m_formatter = new Utils::OutputFormatter;
    m_formatter->setPlainTextEdit(m_ui->messageEdit);
}

SimulatorOperationDialog::~SimulatorOperationDialog()
{
    // Cancel all pending futures.
    foreach (auto watcher, m_futureWatchList) {
        if (!watcher->isFinished())
            watcher->cancel();
    }

    // wait for futures to finish
    foreach (auto watcher, m_futureWatchList) {
        if (!watcher->isFinished())
            watcher->waitForFinished();
        delete watcher;
    }

    delete m_formatter;
    delete m_ui;
}

void SimulatorOperationDialog::addFutures(const QList<QFuture<void> > &futureList)
{
    foreach (auto future, futureList) {
        if (!future.isFinished() || !future.isCanceled()) {
            auto watcher = new QFutureWatcher<void>;
            watcher->setFuture(future);
            connect(watcher, &QFutureWatcher<void>::finished,
                    this, &SimulatorOperationDialog::futureFinished);
            m_futureWatchList << watcher;
        }
    }
    updateInputs();
}

void SimulatorOperationDialog::addMessage(const QString &message, Utils::OutputFormat format)
{
    m_formatter->appendMessage(message + "\n\n", format);
}

void SimulatorOperationDialog::addMessage(const SimulatorInfo &siminfo,
                                       const SimulatorControl::ResponseData &response,
                                       const QString &context)
{
    QTC_CHECK(siminfo.identifier == response.simUdid);
    if (response.success) {
        addMessage(tr("%1, %2\nOperation %3 completed successfully.").arg(siminfo.name)
                   .arg(siminfo.runtimeName).arg(context), Utils::StdOutFormat);
    } else {
        QByteArray erroMsg = response.commandOutput.trimmed();
        QString message = tr("%1, %2\nOperation %3 failed.\nUDID: %4\nError: %5").arg(siminfo.name)
                .arg(siminfo.runtimeName).arg(context).arg(siminfo.identifier)
                .arg(erroMsg.isEmpty() ? tr("Unknown") : QString::fromUtf8(erroMsg));
        addMessage(message, Utils::StdErrFormat);
        qCDebug(iosCommon) << message;
    }
}

void SimulatorOperationDialog::futureFinished()
{
    auto watcher = static_cast<QFutureWatcher<void> *>(sender());
    m_futureWatchList.removeAll(watcher);
    watcher->deleteLater();
    updateInputs();
}

void SimulatorOperationDialog::updateInputs()
{
    bool enableOk = m_futureWatchList.isEmpty();
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(!enableOk);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enableOk);
    if (enableOk) {
        addMessage(tr("Done."), Utils::NormalMessageFormat);
        m_ui->progressBar->setMaximum(1); // Stop progress bar.
    }
}

} // namespace Internal
} // namespace Ios
