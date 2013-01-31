/**************************************************************************
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

#include "crashhandler.h"
#include "crashhandlerdialog.h"
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

CrashHandlerDialog::CrashHandlerDialog(CrashHandler *handler, const QString &signalName,
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

    const QStyle * const style = QApplication::style();
    m_ui->closeButton->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));

    const int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0);
    QIcon icon = style->standardIcon(QStyle::SP_MessageBoxCritical);
    m_ui->iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));

    connect(m_ui->copyToClipBoardButton, SIGNAL(clicked()), this, SLOT(copyToClipboardClicked()));
    connect(m_ui->reportBugButton, SIGNAL(clicked()), m_crashHandler, SLOT(openBugTracker()));
    connect(m_ui->debugAppButton, SIGNAL(clicked()), m_crashHandler, SLOT(debugApplication()));
    connect(m_ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));

    setApplicationInfo(signalName);
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

void CrashHandlerDialog::setApplicationInfo(const QString &signalName)
{
    const QString ideName = QLatin1String("Qt Creator");
    const QString title = tr("%1 has closed unexpectedly (Signal \"%2\")").arg(ideName, signalName);
    const QString introLabelContents = tr(
        "<p><b>%1.</b></p>"
        "<p>Please file a <a href='%2'>bug report</a> with the debug information provided below.</p>")
        .arg(title, QLatin1String(URL_BUGTRACKER));
    m_ui->introLabel->setText(introLabelContents);
    setWindowTitle(title);

    QString revision;
#ifdef IDE_REVISION
     revision = tr(" from revision %1").arg(QString::fromLatin1(Core::Constants::IDE_REVISION_STR).left(10));
#endif
    const QString versionInformation = tr(
        "%1 %2%3, built on %4 at %5, based on Qt %6 (%7 bit)\n")
            .arg(ideName, QLatin1String(Core::Constants::IDE_VERSION_LONG), revision,
                 QLatin1String(__DATE__), QLatin1String(__TIME__), QLatin1String(QT_VERSION_STR),
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
