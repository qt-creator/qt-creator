// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simulatoroperationdialog.h"

#include "iostr.h"

#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>

namespace Ios::Internal {

static Q_LOGGING_CATEGORY(iosCommon, "qtc.ios.common", QtWarningMsg)

SimulatorOperationDialog::SimulatorOperationDialog(QWidget *parent) :
    // TODO: Maximize buttong only because of QTBUG-41932
    QDialog(parent,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint)
{
    resize(580, 320);
    setModal(true);
    setWindowTitle(Tr::tr("Simulator Operation Status"));

    auto messageEdit = new QPlainTextEdit;
    messageEdit->setReadOnly(true);

    m_progressBar = new QProgressBar;
    m_progressBar->setMaximum(0);
    m_progressBar->setValue(-1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_formatter = new Utils::OutputFormatter;
    m_formatter->setPlainTextEdit(messageEdit);

    using namespace Layouting;

    Column {
        messageEdit,
        m_progressBar,
        m_buttonBox
    }.attachTo(this);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SimulatorOperationDialog::~SimulatorOperationDialog()
{
    // Cancel all pending futures.
    const auto futureWatchList = m_futureWatchList;
    for (auto watcher : futureWatchList) {
        if (!watcher->isFinished())
            watcher->cancel();
    }

    // wait for futures to finish
    for (auto watcher : futureWatchList) {
        if (!watcher->isFinished())
            watcher->waitForFinished();
        delete watcher;
    }

    delete m_formatter;
}

void SimulatorOperationDialog::addFutures(const QList<QFuture<void> > &futureList)
{
    for (auto future : futureList) {
        if (!future.isFinished() || !future.isCanceled()) {
            auto watcher = new QFutureWatcher<void>;
            connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
                m_futureWatchList.removeAll(watcher);
                watcher->deleteLater();
                updateInputs();
            });
            watcher->setFuture(future);
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
                                          const SimulatorControl::Response &response,
                                          const QString &context)
{
    if (response) {
        QTC_CHECK(siminfo.identifier == response->simUdid);
        addMessage(Tr::tr("%1, %2\nOperation %3 completed successfully.").arg(siminfo.name)
                   .arg(siminfo.runtimeName).arg(context), Utils::StdOutFormat);
    } else {
        QString erroMsg = response.error();
        QString message = Tr::tr("%1, %2\nOperation %3 failed.\nUDID: %4\nError: %5").arg(siminfo.name)
                .arg(siminfo.runtimeName).arg(context).arg(siminfo.identifier)
                .arg(erroMsg.isEmpty() ? Tr::tr("Unknown") : erroMsg);
        addMessage(message, Utils::StdErrFormat);
        qCDebug(iosCommon) << message;
    }
}

void SimulatorOperationDialog::updateInputs()
{
    bool enableOk = m_futureWatchList.isEmpty();
    m_buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(!enableOk);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enableOk);
    if (enableOk) {
        addMessage(Tr::tr("Done."), Utils::NormalMessageFormat);
        m_progressBar->setMaximum(1); // Stop progress bar.
    }
}

} // Ios::Internal
