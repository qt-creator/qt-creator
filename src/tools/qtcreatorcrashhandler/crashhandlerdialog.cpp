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

#include "crashhandlerdialog.h"
#include "crashhandler.h"
#include "ui_crashhandlerdialog.h"
#include "utils.h"

#include <app/app_version.h>
#include <utils/checkablemessagebox.h>

#include <QClipboard>
#include <QIcon>
#include <QSettings>

static const char SettingsApplication[] = "QtCreator";
static const char SettingsKeySkipWarningAbortingBacktrace[]
    = "CrashHandler/SkipWarningAbortingBacktrace";

CrashHandlerDialog::CrashHandlerDialog(CrashHandler *handler,
                                       const QString &signalName,
                                       const QString &appName,
                                       QWidget *parent) :
    QDialog(parent),
    m_crashHandler(handler),
    m_ui(new Ui::CrashHandlerDialog)
{
    m_ui->setupUi(this);
    m_ui->introLabel->setTextFormat(Qt::RichText);
    m_ui->introLabel->setOpenExternalLinks(true);
    m_ui->debugInfoEdit->setReadOnly(true);
    m_ui->progressBar->setMinimum(0);
    m_ui->progressBar->setMaximum(0);
    m_ui->restartAppCheckBox->setText(tr("&Restart %1 on close").arg(appName));

    const QStyle * const style = QApplication::style();
    m_ui->closeButton->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));

    const int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0);
    QIcon icon = style->standardIcon(QStyle::SP_MessageBoxCritical);
    m_ui->iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));

    connect(m_ui->copyToClipBoardButton, &QAbstractButton::clicked,
            this, &CrashHandlerDialog::copyToClipboardClicked);
    connect(m_ui->reportBugButton, &QAbstractButton::clicked,
            m_crashHandler, &CrashHandler::openBugTracker);
    connect(m_ui->debugAppButton, &QAbstractButton::clicked,
            m_crashHandler, &CrashHandler::debugApplication);
    connect(m_ui->closeButton, &QAbstractButton::clicked, this, &CrashHandlerDialog::close);

    setApplicationInfo(signalName, appName);
}

CrashHandlerDialog::~CrashHandlerDialog()
{
    delete m_ui;
}

bool CrashHandlerDialog::runDebuggerWhileBacktraceNotFinished()
{
    // Check settings.
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
        QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
        QLatin1String(SettingsApplication));
    if (settings.value(QLatin1String(SettingsKeySkipWarningAbortingBacktrace), false).toBool())
        return true;

    // Ask user.
    const QString title = tr("Run Debugger And Abort Collecting Backtrace?");
    const QString message = tr(
        "<html><head/><body>"
          "<p><b>Run the debugger and abort collecting backtrace?</b></p>"
          "<p>You have requested to run the debugger while collecting the backtrace was not "
          "finished.</p>"
        "</body></html>");
    const QString checkBoxText = tr("Do not &ask again.");
    bool checkBoxSetting = false;
    const QDialogButtonBox::StandardButton button = Utils::CheckableMessageBox::question(this,
        title, message, checkBoxText, &checkBoxSetting,
        QDialogButtonBox::Yes|QDialogButtonBox::No, QDialogButtonBox::No);
    if (checkBoxSetting)
        settings.setValue(QLatin1String(SettingsKeySkipWarningAbortingBacktrace), checkBoxSetting);

    return button == QDialogButtonBox::Yes;
}

void CrashHandlerDialog::setToFinalState()
{
    m_ui->progressBar->hide();
    m_ui->copyToClipBoardButton->setEnabled(true);
    m_ui->reportBugButton->setEnabled(true);
}

void CrashHandlerDialog::disableRestartAppCheckBox()
{
    m_ui->restartAppCheckBox->setDisabled(true);
}

void CrashHandlerDialog::disableDebugAppButton()
{
    m_ui->debugAppButton->setDisabled(true);
}

void CrashHandlerDialog::setApplicationInfo(const QString &signalName, const QString &appName)
{
    const QString title = tr("%1 has closed unexpectedly (Signal \"%2\")").arg(appName, signalName);
    const QString introLabelContents = tr(
        "<p><b>%1.</b></p>"
        "<p>Please file a <a href='%2'>bug report</a> with the debug information provided below.</p>")
        .arg(title, QLatin1String(URL_BUGTRACKER));
    m_ui->introLabel->setText(introLabelContents);
    setWindowTitle(title);

    QString revision;
#ifdef IDE_REVISION
     revision = QLatin1Char(' ') + tr("from revision %1").arg(QString::fromLatin1(Core::Constants::IDE_REVISION_STR).left(10));
#endif
    const QString versionInformation = tr(
        "%1 %2%3, based on Qt %4 (%5 bit)\n")
            .arg(appName, QLatin1String(Core::Constants::IDE_VERSION_LONG), revision,
                 QLatin1String(QT_VERSION_STR),
                 QString::number(QSysInfo::WordSize));
    m_ui->debugInfoEdit->append(versionInformation);
}

void CrashHandlerDialog::appendDebugInfo(const QString &chunk)
{
    m_ui->debugInfoEdit->append(chunk);
}

void CrashHandlerDialog::selectLineWithContents(const QString &text)
{
    // The selected line will be the first line visible.

    // Go to end.
    QTextCursor cursor = m_ui->debugInfoEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_ui->debugInfoEdit->setTextCursor(cursor);

    // Find text by searching backwards.
    m_ui->debugInfoEdit->find(text, QTextDocument::FindCaseSensitively | QTextDocument::FindBackward);

    // Highlight whole line.
    cursor = m_ui->debugInfoEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    m_ui->debugInfoEdit->setTextCursor(cursor);
}

void CrashHandlerDialog::copyToClipboardClicked()
{
    QApplication::clipboard()->setText(m_ui->debugInfoEdit->toPlainText());
}

void CrashHandlerDialog::close()
{
    if (m_ui->restartAppCheckBox->isEnabled() && m_ui->restartAppCheckBox->isChecked())
        m_crashHandler->restartApplication();
    qApp->quit();
}
