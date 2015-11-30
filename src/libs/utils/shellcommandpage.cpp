/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "shellcommandpage.h"
#include "shellcommand.h"
#include "outputformatter.h"
#include "qtcassert.h"
#include "theme/theme.h"

#include <QAbstractButton>
#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

/*!
    \class Utils::ShellCommandPage

    \brief The ShellCommandPage implements a page showing the
    progress of a \c ShellCommand.

    Turns complete when the command succeeds.
*/

namespace Utils {

ShellCommandPage::ShellCommandPage(QWidget *parent) :
    WizardPage(parent),
    m_startedStatus(tr("Command started...")),
    m_overwriteOutput(false),
    m_state(Idle)
{
    resize(264, 200);
    auto verticalLayout = new QVBoxLayout(this);
    m_logPlainTextEdit = new QPlainTextEdit;
    m_formatter = new Utils::OutputFormatter;
    m_logPlainTextEdit->setReadOnly(true);
    m_formatter->setPlainTextEdit(m_logPlainTextEdit);

    verticalLayout->addWidget(m_logPlainTextEdit);

    m_statusLabel = new QLabel;
    verticalLayout->addWidget(m_statusLabel);
    setTitle(tr("Run Command"));
}

ShellCommandPage::~ShellCommandPage()
{
    QTC_ASSERT(m_state != Running, QApplication::restoreOverrideCursor());
    delete m_formatter;
}

void ShellCommandPage::setStartedStatus(const QString &startedStatus)
{
    m_startedStatus = startedStatus;
}

void ShellCommandPage::start(ShellCommand *command)
{
    if (!command) {
        m_logPlainTextEdit->setPlainText(tr("No job running, please abort."));
        return;
    }

    QTC_ASSERT(m_state != Running, return);
    m_command = command;
    command->setProgressiveOutput(true);
    connect(command, &ShellCommand::stdOutText, this, &ShellCommandPage::reportStdOut);
    connect(command, &ShellCommand::stdErrText, this, &ShellCommandPage::reportStdErr);
    connect(command, &ShellCommand::finished, this, &ShellCommandPage::slotFinished);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_logPlainTextEdit->clear();
    m_overwriteOutput = false;
    m_statusLabel->setText(m_startedStatus);
    m_statusLabel->setPalette(QPalette());
    m_state = Running;
    command->execute();

    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

void ShellCommandPage::slotFinished(bool ok, int exitCode, const QVariant &)
{
    QTC_ASSERT(m_state == Running, return);

    const bool success = (ok && exitCode == 0);
    QString message;
    QPalette palette;

    if (success) {
        m_state = Succeeded;
        message = tr("Succeeded.");
        palette.setColor(QPalette::WindowText, creatorTheme()->color(Theme::TextColorNormal).name());
    } else {
        m_state = Failed;
        message = tr("Failed.");
        palette.setColor(QPalette::WindowText, creatorTheme()->color(Theme::TextColorError).name());
    }

    m_statusLabel->setText(message);
    m_statusLabel->setPalette(palette);

    QApplication::restoreOverrideCursor();
    wizard()->button(QWizard::BackButton)->setEnabled(true);

    if (success)
        emit completeChanged();
    emit finished(success);
}

void ShellCommandPage::reportStdOut(const QString &text)
{
    m_formatter->appendMessage(text, Utils::StdOutFormat);
}

void ShellCommandPage::reportStdErr(const QString &text)
{
    m_formatter->appendMessage(text, Utils::StdErrFormat);
}

void ShellCommandPage::terminate()
{
    if (m_command)
        m_command->cancel();
}

bool ShellCommandPage::handleReject()
{
    if (!isRunning())
        return false;

    terminate();
    return true;
}

bool ShellCommandPage::isComplete() const
{
    return m_state == Succeeded;
}

} // namespace Utils
