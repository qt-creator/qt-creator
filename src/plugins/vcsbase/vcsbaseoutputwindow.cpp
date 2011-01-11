/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "vcsbaseoutputwindow.h"

#include <utils/qtcassert.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCharFormat>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlockUserData>

#include <QtCore/QPointer>
#include <QtCore/QTextCodec>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QPoint>
#include <QtCore/QFileInfo>

namespace VCSBase {

namespace Internal {

// Store repository along with text blocks
class RepositoryUserData : public QTextBlockUserData    {
public:
    explicit RepositoryUserData(const QString &repo) : m_repository(repo) {}
    const QString &repository() const { return m_repository; }

private:
    const QString m_repository;
};

// A plain text edit with a special context menu containing "Clear" and
// and functions to append specially formatted entries.
class OutputWindowPlainTextEdit : public QPlainTextEdit {
public:
    explicit OutputWindowPlainTextEdit(QWidget *parent = 0);

    void appendLines(QString s, const QString &repository = QString());
    // Append red error text and pop up.
    void appendError(const QString &text);
    // Append warning error text and pop up.
    void appendWarning(const QString &text);
    // Append a bold command "10:00 " + "Executing: vcs -diff"
    void appendCommand(const QString &text);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    QString identifierUnderCursor(const QPoint &pos, QString *repository = 0) const;

    const QTextCharFormat m_defaultFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_warningFormat;
    QTextCharFormat m_commandFormat;
};

OutputWindowPlainTextEdit::OutputWindowPlainTextEdit(QWidget *parent) :
    QPlainTextEdit(parent),
    m_defaultFormat(currentCharFormat()),
    m_errorFormat(m_defaultFormat),
    m_warningFormat(m_defaultFormat),
    m_commandFormat(m_defaultFormat)
{
    setReadOnly(true);
    setFrameStyle(QFrame::NoFrame);
    m_errorFormat.setForeground(Qt::red);
    m_warningFormat.setForeground(Qt::darkYellow);
    m_commandFormat.setFontWeight(QFont::Bold);
}

// Search back for beginning of word
static inline int firstWordCharacter(const QString &s, int startPos)
{
    for ( ; startPos >= 0 ; startPos--) {
        if (s.at(startPos).isSpace())
            return startPos + 1;
    }
    return 0;
}

QString OutputWindowPlainTextEdit::identifierUnderCursor(const QPoint &widgetPos, QString *repository) const
{
    if (repository)
        repository->clear();
    // Get the blank-delimited word under cursor. Note that
    // using "SelectWordUnderCursor" does not work since it breaks
    // at delimiters like '/'. Get the whole line
    QTextCursor cursor = cursorForPosition(widgetPos);
    const int cursorDocumentPos = cursor.position();
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    QString block = cursor.selectedText();
    // Determine cursor position within line and find blank-delimited word
    const int cursorPos = cursorDocumentPos - cursor.block().position();
    const int blockSize = block.size();
    if (cursorPos < 0 || cursorPos >= blockSize || block.at(cursorPos).isSpace())
        return QString();
    // Retrieve repository if desired
    if (repository)
        if (QTextBlockUserData *data = cursor.block().userData())
            *repository = static_cast<const RepositoryUserData*>(data)->repository();
    // Find first non-space character of word and find first non-space character past
    const int startPos = firstWordCharacter(block, cursorPos);
    int endPos = cursorPos;
    for ( ; endPos < blockSize && !block.at(endPos).isSpace(); endPos++) ;
    return endPos > startPos ? block.mid(startPos, endPos - startPos) : QString();
}

void OutputWindowPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    // Add 'open file'
    QString repository;
    const QString token = identifierUnderCursor(event->pos(), &repository);
    QAction *openAction = 0;
    if (!token.isEmpty()) {
        // Check for a file, expand via repository if relative
        QFileInfo fi(token);
        if (!repository.isEmpty() && !fi.isFile() && fi.isRelative())
            fi = QFileInfo(repository + QLatin1Char('/') + token);
        if (fi.isFile())  {
            menu->addSeparator();
            openAction = menu->addAction(VCSBaseOutputWindow::tr("Open \"%1\"").
                                         arg(QDir::toNativeSeparators(fi.fileName())));
            openAction->setData(fi.absoluteFilePath());
        }
    }
    // Add 'clear'
    menu->addSeparator();
    QAction *clearAction = menu->addAction(VCSBaseOutputWindow::tr("Clear"));

    // Run
    QAction *action = menu->exec(event->globalPos());
    if (action) {
        if (action == clearAction) {
            clear();
            return;
        }
        if (action == openAction) {
            const QString fileName = action->data().toString();
            Core::EditorManager::instance()->openEditor(fileName, QString(), Core::EditorManager::ModeSwitch);
        }
    }
    delete menu;
}

void OutputWindowPlainTextEdit::appendLines(QString s, const QString &repository)
{
    if (s.isEmpty())
        return;
    // Avoid additional new line character generated by appendPlainText
    if (s.endsWith(QLatin1Char('\n')))
        s.truncate(s.size() - 1);
    const int previousLineCount = document()->lineCount();
    appendPlainText(s);
    // Scroll down
    moveCursor(QTextCursor::End);
    ensureCursorVisible();
    if (!repository.isEmpty()) {
        // Associate repository with new data.
        QTextBlock block = document()->findBlockByLineNumber(previousLineCount);
        for ( ; block.isValid(); block = block.next())
            block.setUserData(new RepositoryUserData(repository));
    }
}

void OutputWindowPlainTextEdit::appendError(const QString &text)
{
    setCurrentCharFormat(m_errorFormat);
    appendLines(text);
    setCurrentCharFormat(m_defaultFormat);
}

void OutputWindowPlainTextEdit::appendWarning(const QString &text)
{
    setCurrentCharFormat(m_warningFormat);
    appendLines(text);
    setCurrentCharFormat(m_defaultFormat);
}

// Append command with new line and log time stamp
void OutputWindowPlainTextEdit::appendCommand(const QString &text)
{
    setCurrentCharFormat(m_commandFormat);
    const QString timeStamp = QTime::currentTime().toString(QLatin1String("\nHH:mm "));
    appendLines(timeStamp + text);
    setCurrentCharFormat(m_defaultFormat);
}

} // namespace Internal

// ------------------- VCSBaseOutputWindowPrivate
struct VCSBaseOutputWindowPrivate {
    static VCSBaseOutputWindow *instance;
    Internal::OutputWindowPlainTextEdit *plainTextEdit();

    QPointer<Internal::OutputWindowPlainTextEdit> m_plainTextEdit;
    QString repository;
};

// Create log editor on demand. Some errors might be logged
// before CorePlugin::extensionsInitialized() pulls up the windows.

Internal::OutputWindowPlainTextEdit *VCSBaseOutputWindowPrivate::plainTextEdit()
{
    if (!m_plainTextEdit)
        m_plainTextEdit = new Internal::OutputWindowPlainTextEdit();
    return m_plainTextEdit;
}

VCSBaseOutputWindow *VCSBaseOutputWindowPrivate::instance = 0;

VCSBaseOutputWindow::VCSBaseOutputWindow() :
    d(new VCSBaseOutputWindowPrivate)
{
    VCSBaseOutputWindowPrivate::instance = this;
}

VCSBaseOutputWindow::~VCSBaseOutputWindow()
{
    VCSBaseOutputWindowPrivate::instance = 0;
    delete d;
}

QWidget *VCSBaseOutputWindow::outputWidget(QWidget *parent)
{
    if (d->m_plainTextEdit) {
        if (parent != d->m_plainTextEdit->parent())
            d->m_plainTextEdit->setParent(parent);
    } else {
        d->m_plainTextEdit = new Internal::OutputWindowPlainTextEdit(parent);
    }
    return d->m_plainTextEdit;
}

QWidgetList VCSBaseOutputWindow::toolBarWidgets() const
{
    return QWidgetList();
}

QString VCSBaseOutputWindow::displayName() const
{
    return tr("Version Control");
}

int VCSBaseOutputWindow::priorityInStatusBar() const
{
    return -1;
}

void VCSBaseOutputWindow::clearContents()
{
    if (d->m_plainTextEdit)
        d->m_plainTextEdit->clear();
}

void VCSBaseOutputWindow::visibilityChanged(bool visible)
{
    if (visible && d->m_plainTextEdit)
        d->m_plainTextEdit->setFocus();
}

void VCSBaseOutputWindow::setFocus()
{
}

bool VCSBaseOutputWindow::hasFocus()
{
    return false;
}

bool VCSBaseOutputWindow::canFocus()
{
    return false;
}

bool VCSBaseOutputWindow::canNavigate()
{
    return false;
}

bool VCSBaseOutputWindow::canNext()
{
    return false;
}

bool VCSBaseOutputWindow::canPrevious()
{
    return false;
}

void VCSBaseOutputWindow::goToNext()
{
}

void VCSBaseOutputWindow::goToPrev()
{
}

void VCSBaseOutputWindow::setText(const QString &text)
{
    d->plainTextEdit()->setPlainText(text);
}

void VCSBaseOutputWindow::setData(const QByteArray &data)
{
    setText(QTextCodec::codecForLocale()->toUnicode(data));
}

void VCSBaseOutputWindow::appendSilently(const QString &text)
{
    d->plainTextEdit()->appendLines(text, d->repository);
}

void VCSBaseOutputWindow::append(const QString &text)
{
    appendSilently(text);
    // Pop up without focus
    if (!d->plainTextEdit()->isVisible())
        popup(false);
}

void VCSBaseOutputWindow::appendError(const QString &text)
{
    d->plainTextEdit()->appendError(text);
    if (!d->plainTextEdit()->isVisible())
        popup(false); // Pop up without focus
}

void VCSBaseOutputWindow::appendWarning(const QString &text)
{
    d->plainTextEdit()->appendWarning(text);
    if (!d->plainTextEdit()->isVisible())
        popup(false); // Pop up without focus
}

// Helper to format arguments for log windows hiding common password
// options.
static inline QString formatArguments(const QStringList &args)
{
    const char passwordOptionC[] = "--password";

    QString rc;
    QTextStream str(&rc);
    const int size = args.size();
    // Skip authentication options
    for (int i = 0; i < size; i++) {
        const QString &arg = args.at(i);
        if (i)
            str << ' ';
        str << arg;
        if (arg == QLatin1String(passwordOptionC)) {
            str << " ********";
            i++;
        }
    }
    return rc;
}

QString VCSBaseOutputWindow::msgExecutionLogEntry(const QString &workingDir,
                                                  const QString &executable,
                                                  const QStringList &arguments)
{
    const QString args = formatArguments(arguments);
    const QString nativeExecutable = QDir::toNativeSeparators(executable);
    if (workingDir.isEmpty())
        return tr("Executing: %1 %2\n").arg(nativeExecutable, args);
    return tr("Executing in %1: %2 %3\n").
            arg(QDir::toNativeSeparators(workingDir), nativeExecutable, args);
}

void VCSBaseOutputWindow::appendCommand(const QString &text)
{
    d->plainTextEdit()->appendCommand(text);
}

void VCSBaseOutputWindow::appendCommand(const QString &workingDirectory,
                                        const QString &binary,
                                        const QStringList &args)
{
    appendCommand(msgExecutionLogEntry(workingDirectory, binary, args));
}


void VCSBaseOutputWindow::appendData(const QByteArray &data)
{
    appendDataSilently(data);
    if (!d->plainTextEdit()->isVisible())
        popup(false); // Pop up without focus
}

void VCSBaseOutputWindow::appendDataSilently(const QByteArray &data)
{
    appendSilently(QTextCodec::codecForLocale()->toUnicode(data));
}

VCSBaseOutputWindow *VCSBaseOutputWindow::instance()
{
    if (!VCSBaseOutputWindowPrivate::instance) {
        VCSBaseOutputWindow *w = new VCSBaseOutputWindow;
        Q_UNUSED(w)
    }
    return VCSBaseOutputWindowPrivate::instance;
}

QString VCSBaseOutputWindow::repository() const
{
    return d->repository;
}

void VCSBaseOutputWindow::setRepository(const QString &r)
{
    d->repository = r;
}

void VCSBaseOutputWindow::clearRepository()
{
    d->repository.clear();
}

} // namespace VCSBase
