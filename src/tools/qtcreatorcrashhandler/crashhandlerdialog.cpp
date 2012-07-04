/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "crashhandler.h"
#include "crashhandlerdialog.h"
#include "ui_crashhandlerdialog.h"
#include "utils.h"

#include <app/app_version.h>

#include <QClipboard>
#include <QIcon>

CrashHandlerDialog::CrashHandlerDialog(CrashHandler *handler, QWidget *parent) :
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
    connect(m_ui->restartAppButton, SIGNAL(clicked()), m_crashHandler, SLOT(restartApplication()));
    connect(m_ui->closeButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    setApplicationInfo();
}

CrashHandlerDialog::~CrashHandlerDialog()
{
    delete m_ui;
}

void CrashHandlerDialog::setToFinalState()
{
    m_ui->progressBar->hide();
    m_ui->copyToClipBoardButton->setEnabled(true);
    m_ui->reportBugButton->setEnabled(true);
}

void CrashHandlerDialog::disableRestartAppButton()
{
    m_ui->restartAppButton->setDisabled(true);
}

void CrashHandlerDialog::setApplicationInfo()
{
    const QString ideName = QLatin1String("Qt Creator");
    const QString contents = tr(
        "<p><b>%1 has closed unexpectedly.</b></p>"
        "<p>Please file a <a href='%2'>bug report</a> with the debug information provided below.</p>")
        .arg(ideName, QLatin1String(URL_BUGTRACKER));
    m_ui->introLabel->setText(contents);

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
