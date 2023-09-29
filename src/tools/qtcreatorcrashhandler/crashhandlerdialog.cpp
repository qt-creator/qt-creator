// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "crashhandlerdialog.h"

#include "crashhandler.h"
#include "utils.h"

#include <app/app_version.h>

#include <utils/checkablemessagebox.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextEdit>

static const char SettingsApplication[] = "QtCreator";
static const char SettingsKeySkipWarningAbortingBacktrace[]
    = "CrashHandler/SkipWarningAbortingBacktrace";

namespace {

class StacktraceHighlighter : public QSyntaxHighlighter
{
public:
    StacktraceHighlighter(QTextDocument *parent = 0)
        : QSyntaxHighlighter(parent)
    {
        m_currentThreadFormat.setFontWeight(QFont::Bold);
        m_currentThreadFormat.setForeground(Qt::red);

        m_threadFormat.setFontWeight(QFont::Bold);
        m_threadFormat.setForeground(Qt::blue);

        m_stackTraceNumberFormat.setFontWeight(QFont::Bold);
        m_stackTraceFunctionFormat.setFontWeight(QFont::Bold);
        m_stackTraceLocationFormat.setForeground(Qt::darkGreen);
    }

protected:
    void highlightBlock(const QString &text) override
    {
        if (text.isEmpty())
            return;

        // Current thread line
        // [Current thread is 1 (Thread 0x7f8212984f00 (LWP 20425))]
        if (text.startsWith("[Current thread is"))
            return setFormat(0, text.size(), m_currentThreadFormat);

        // Thread line
        // Thread 1 (Thread 0x7fc24eb7ef00 (LWP 18373)):
        if (text.startsWith("Thread"))
            return setFormat(0, text.size(), m_threadFormat);

        // Stack trace line
        // #3  0x000055d001160992 in main (argc=2, argv=0x7ffd0b4873a8) at file.cpp:82
        if (text.startsWith('#')) {
            int startIndex = -1;
            int endIndex = -1;

            // Item number
            endIndex = text.indexOf(' ');
            if (endIndex != -1)
                setFormat(0, endIndex, m_stackTraceNumberFormat);

            // Function name
            startIndex = text.indexOf(" in ");
            if (startIndex != -1) {
                startIndex += 4;
                const QString stopCharacters = "(<"; // Do not include function/template parameters
                endIndex = startIndex;
                while (endIndex < text.size() && !stopCharacters.contains(text[endIndex]))
                    ++endIndex;
                setFormat(startIndex, endIndex - startIndex, m_stackTraceFunctionFormat);
            }

            // Location
            startIndex = text.indexOf("at ");
            if (startIndex != -1)
                setFormat(startIndex + 3, text.size() - startIndex, m_stackTraceLocationFormat);
        }
    }

private:
    QTextCharFormat m_currentThreadFormat;
    QTextCharFormat m_threadFormat;

    QTextCharFormat m_stackTraceNumberFormat;
    QTextCharFormat m_stackTraceFunctionFormat;
    QTextCharFormat m_stackTraceLocationFormat;
};

} // anonymous

class CrashHandlerDialogPrivate
{
    Q_DECLARE_TR_FUNCTIONS(CrashHandlerDialog)

public:
    CrashHandlerDialogPrivate(CrashHandlerDialog *dialog, CrashHandler *handler)
        : q(dialog)
        , m_crashHandler(handler)
        , m_iconLabel(new QLabel(q))
        , m_introLabel(new QLabel(q))
        , m_progressBar(new QProgressBar(q))
        , m_debugInfoEdit(new QTextEdit(q))
        , m_restartAppCheckBox(new QCheckBox(q))
        , m_clipboardButton(new QPushButton(tr("C&opy to clipboard"), q))
        , m_reportButton(new QPushButton(tr("Report this &bug"), q))
        , m_debugButton(new QPushButton(tr("Attach and &Debug"), q))
        , m_closeButton(new QPushButton(tr("&Close"), q))
    {
        m_introLabel->setTextFormat(Qt::RichText);
        m_introLabel->setOpenExternalLinks(true);
        m_introLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                              Qt::TextSelectableByMouse);

        const QStyle * const style = QApplication::style();
        const int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0);
        const QIcon icon = style->standardIcon(QStyle::SP_MessageBoxCritical);
        m_iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));

        m_progressBar->setMinimum(0);
        m_progressBar->setMaximum(0);

        m_debugInfoEdit->setReadOnly(true);
        new StacktraceHighlighter(m_debugInfoEdit->document());

        m_restartAppCheckBox->setChecked(true);
        m_clipboardButton->setToolTip(tr("Copy the whole contents to clipboard."));
        m_clipboardButton->setEnabled(false);
        m_reportButton->setToolTip(tr("Open the bug tracker web site."));
        m_reportButton->setEnabled(false);
        m_debugButton->setToolTip(tr("Debug the application with a new instance of Qt Creator. "
                                        "During debugging the crash handler will be hidden."));
        m_closeButton->setToolTip(tr("Quit the handler and the crashed application."));
        m_closeButton->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));

        QObject::connect(m_clipboardButton, &QAbstractButton::clicked, q, [this] {
            Utils::setClipboardAndSelection(m_debugInfoEdit->toPlainText());
        });
        QObject::connect(m_reportButton, &QAbstractButton::clicked,
                         m_crashHandler, &CrashHandler::openBugTracker);
        QObject::connect(m_debugButton, &QAbstractButton::clicked,
                         m_crashHandler, &CrashHandler::debugApplication);
        QObject::connect(m_closeButton, &QAbstractButton::clicked, q, [this] {
            if (m_restartAppCheckBox->isEnabled() && m_restartAppCheckBox->isChecked())
                m_crashHandler->restartApplication();
            QCoreApplication::quit();
        });

        using namespace Layouting;

        Column {
            Row { m_iconLabel, m_introLabel, st },
            m_progressBar,
            m_debugInfoEdit,
            m_restartAppCheckBox,
            Row { m_clipboardButton, m_reportButton, st, m_debugButton, m_closeButton }
        }.attachTo(q);
    }

    CrashHandlerDialog *q = nullptr;

    CrashHandler *m_crashHandler = nullptr;

    QLabel *m_iconLabel = nullptr;
    QLabel *m_introLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTextEdit *m_debugInfoEdit = nullptr;
    QCheckBox *m_restartAppCheckBox = nullptr;
    QPushButton *m_clipboardButton = nullptr;
    QPushButton *m_reportButton = nullptr;
    QPushButton *m_debugButton = nullptr;
    QPushButton *m_closeButton = nullptr;
};

CrashHandlerDialog::CrashHandlerDialog(CrashHandler *handler, const QString &signalName,
                                       const QString &appName, QWidget *parent)
    : QDialog(parent)
    , d(new CrashHandlerDialogPrivate(this, handler))
{
    d->m_restartAppCheckBox->setText(tr("&Restart %1 on close").arg(appName));
    setApplicationInfo(signalName, appName);
    resize(600, 800);
}

CrashHandlerDialog::~CrashHandlerDialog()
{
    delete d;
}

bool CrashHandlerDialog::runDebuggerWhileBacktraceNotFinished()
{
    // Check settings.
    Utils::QtcSettings settings(QSettings::IniFormat,
                                QSettings::UserScope,
                                QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                                QLatin1String(SettingsApplication));
    Utils::CheckableMessageBox::initialize(&settings);

    // Ask user.
    const QString title = tr("Run Debugger And Abort Collecting Backtrace?");
    const QString message = tr(
        "<html><head/><body>"
          "<p><b>Run the debugger and abort collecting backtrace?</b></p>"
          "<p>You have requested to run the debugger while collecting the backtrace was not "
          "finished.</p>"
        "</body></html>");

    const QMessageBox::StandardButton button
        = Utils::CheckableMessageBox::question(this,
                                               title,
                                               message,
                                               Utils::Key(SettingsKeySkipWarningAbortingBacktrace),
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::No);

    return button == QMessageBox::Yes;
}

void CrashHandlerDialog::setToFinalState()
{
    d->m_progressBar->hide();
    d->m_clipboardButton->setEnabled(true);
    d->m_reportButton->setEnabled(true);
}

void CrashHandlerDialog::disableRestartAppCheckBox()
{
    d->m_restartAppCheckBox->setDisabled(true);
}

void CrashHandlerDialog::disableDebugAppButton()
{
    d->m_debugButton->setDisabled(true);
}

void CrashHandlerDialog::setApplicationInfo(const QString &signalName, const QString &appName)
{
    const QString title = tr("%1 has closed unexpectedly (Signal \"%2\")").arg(appName, signalName);
    const QString introLabelContents = tr("<p><b>%1.</b></p>"
        "<p>Please file a <a href='%2'>bug report</a> with the debug information provided below.</p>")
        .arg(title, QLatin1String(URL_BUGTRACKER));
    d->m_introLabel->setText(introLabelContents);
    setWindowTitle(title);

    QString revision;
#ifdef IDE_REVISION
    revision = QLatin1Char(' ') + tr("from revision %1")
              .arg(QString::fromLatin1(Core::Constants::IDE_REVISION_STR).left(10));
#endif
    const QString versionInformation = tr("%1 %2%3, based on Qt %4 (%5 bit)\n")
            .arg(appName, QLatin1String(Core::Constants::IDE_VERSION_LONG), revision,
                 QLatin1String(QT_VERSION_STR), QString::number(QSysInfo::WordSize));
    d->m_debugInfoEdit->append(versionInformation);
}

void CrashHandlerDialog::appendDebugInfo(const QString &chunk)
{
    d->m_debugInfoEdit->append(chunk);
}

void CrashHandlerDialog::selectLineWithContents(const QString &text)
{
    // The selected line will be the first line visible.

    // Go to end.
    QTextCursor cursor = d->m_debugInfoEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    d->m_debugInfoEdit->setTextCursor(cursor);

    // Find text by searching backwards.
    d->m_debugInfoEdit->find(text, QTextDocument::FindCaseSensitively | QTextDocument::FindBackward);

    // Highlight whole line.
    cursor = d->m_debugInfoEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    d->m_debugInfoEdit->setTextCursor(cursor);
}
