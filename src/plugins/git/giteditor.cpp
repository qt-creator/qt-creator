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

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "branchadddialog.h"
#include "gitclient.h"
#include "gitsettings.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "githighlighters.h"

#include <coreplugin/icore.h>
#include <texteditor/textdocument.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsoutputwindow.h>

#include <utils/ansiescapecodehandler.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextCursor>

#define CHANGE_PATTERN "[a-f0-9]{7,40}"

using namespace VcsBase;

namespace Git {
namespace Internal {

class GitLogFilterWidget : public QToolBar
{
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::GitLogFilterWidget);

public:
    GitLogFilterWidget(GitEditorWidget *editor)
    {
        auto addLineEdit = [this](const QString &placeholder,
                const QString &tooltip,
                GitEditorWidget *editor)
        {
            auto lineEdit = new Utils::FancyLineEdit;
            lineEdit->setFiltering(true);
            lineEdit->setToolTip(tooltip);
            lineEdit->setPlaceholderText(placeholder);
            lineEdit->setMaximumWidth(200);
            connect(lineEdit, &QLineEdit::returnPressed,
                    editor, &GitEditorWidget::refresh);
            connect(lineEdit, &Utils::FancyLineEdit::rightButtonClicked,
                    editor, &GitEditorWidget::refresh);
            return lineEdit;
        };
        grepLineEdit = addLineEdit(tr("Filter by message"),
                                   tr("Filter log entries by text in the commit message."),
                                   editor);
        pickaxeLineEdit = addLineEdit(tr("Filter by content"),
                                      tr("Filter log entries by added or removed string."),
                                      editor);
        addWidget(new QLabel(tr("Filter:")));
        addSeparator();
        addWidget(grepLineEdit);
        addSeparator();
        addWidget(pickaxeLineEdit);
        addSeparator();
        caseAction = new QAction(tr("Case Sensitive"), this);
        caseAction->setCheckable(true);
        caseAction->setChecked(true);
        connect(caseAction, &QAction::toggled, editor, &GitEditorWidget::refresh);
        addAction(caseAction);
        hide();
        connect(editor, &GitEditorWidget::toggleFilters, this, &QWidget::setVisible);
    }

    Utils::FancyLineEdit *grepLineEdit;
    Utils::FancyLineEdit *pickaxeLineEdit;
    QAction *caseAction;
};

GitEditorWidget::GitEditorWidget() :
    m_changeNumberPattern(QRegularExpression::anchoredPattern(CHANGE_PATTERN))
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), return);
    /* Diff format:
        diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp
        index 40997ff..4e49337 100644
        --- a/src/plugins/git/giteditor.cpp
        +++ b/src/plugins/git/giteditor.cpp
    */
    setDiffFilePattern("^(?:diff --git a/|index |[+-]{3} (?:/dev/null|[ab]/(.+$)))");
    setLogEntryPattern("^commit ([0-9a-f]{8})[0-9a-f]{32}");
    setAnnotateRevisionTextFormat(tr("&Blame %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Blame &Parent Revision %1"));
    setAnnotationEntryPattern("^(" CHANGE_PATTERN ") ");
}

QString GitEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    if (m_changeNumberPattern.match(change).hasMatch())
        return change;
    return QString();
}

BaseAnnotationHighlighter *GitEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new GitAnnotationHighlighter(changes);
}

/* Remove the date specification from annotation, which is tabular:
\code
8ca887aa (author               YYYY-MM-DD HH:MM:SS <offset>  <line>)<content>
\endcode */

static QString sanitizeBlameOutput(const QString &b)
{
    if (b.isEmpty())
        return b;

    const bool omitDate = GitClient::instance()->settings().boolValue(
                GitSettings::omitAnnotationDateKey);
    const QChar space(' ');
    const int parenPos = b.indexOf(')');
    if (parenPos == -1)
        return b;

    int i = parenPos;
    while (i >= 0 && b.at(i) != space)
        --i;
    while (i >= 0 && b.at(i) == space)
        --i;
    int stripPos = i + 1;
    if (omitDate) {
        int spaceCount = 0;
        // i is now on timezone. Go back 3 spaces: That is where the date starts.
        while (i >= 0) {
            if (b.at(i) == space)
                ++spaceCount;
            if (spaceCount == 3) {
                stripPos = i;
                break;
            }
            --i;
        }
    }

    // Copy over the parts that have not changed into a new byte array
    QString result;
    int prevPos = 0;
    int pos = b.indexOf('\n', 0) + 1;
    forever {
        QTC_CHECK(prevPos < pos);
        int afterParen = prevPos + parenPos;
        result.append(b.midRef(prevPos, stripPos));
        result.append(b.midRef(afterParen, pos - afterParen));
        prevPos = pos;
        QTC_CHECK(prevPos != 0);
        if (pos == b.size())
            break;

        pos = b.indexOf('\n', pos) + 1;
        if (pos == 0) // indexOf returned -1
            pos = b.size();
    }
    return result;
}

void GitEditorWidget::setPlainText(const QString &text)
{
    QString modText = text;
    // If desired, filter out the date from annotation
    switch (contentType())
    {
    case LogOutput: {
        Utils::AnsiEscapeCodeHandler handler;
        const QList<Utils::FormattedText> formattedTextList
                = handler.parseText(Utils::FormattedText(text));

        clear();
        QTextCursor cursor = textCursor();
        cursor.beginEditBlock();
        for (const auto &formattedChunk : formattedTextList)
            cursor.insertText(formattedChunk.text, formattedChunk.format);
        cursor.endEditBlock();

        return;
    }
    case AnnotateOutput:
        modText = sanitizeBlameOutput(text);
        break;
    default:
        break;
    }

    textDocument()->setPlainText(modText);
}

void GitEditorWidget::applyDiffChunk(const DiffChunk& chunk, bool revert)
{
    Utils::TemporaryFile patchFile("git-apply-chunk");
    if (!patchFile.open())
        return;

    const QString baseDir = workingDirectory();
    patchFile.write(chunk.header);
    patchFile.write(chunk.chunk);
    patchFile.close();

    QStringList args = {"--cached"};
    if (revert)
        args << "--reverse";
    QString errorMessage;
    if (GitClient::instance()->synchronousApplyPatch(baseDir, patchFile.fileName(), &errorMessage, args)) {
        if (errorMessage.isEmpty())
            VcsOutputWindow::append(tr("Chunk successfully staged"));
        else
            VcsOutputWindow::append(errorMessage);
        if (revert)
            emit diffChunkReverted(chunk);
        else
            emit diffChunkApplied(chunk);
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
}

void GitEditorWidget::init()
{
    VcsBaseEditorWidget::init();
    Utils::Id editorId = textDocument()->id();
    if (editorId == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID)
        textDocument()->setSyntaxHighlighter(new GitSubmitHighlighter);
    else if (editorId == Git::Constants::GIT_REBASE_EDITOR_ID)
        textDocument()->setSyntaxHighlighter(new GitRebaseHighlighter);
}

void GitEditorWidget::addDiffActions(QMenu *menu, const DiffChunk &chunk)
{
    menu->addSeparator();

    QAction *stageAction = menu->addAction(tr("Stage Chunk..."));
    connect(stageAction, &QAction::triggered, this, [this, chunk] {
        applyDiffChunk(chunk, false);
    });

    QAction *unstageAction = menu->addAction(tr("Unstage Chunk..."));
    connect(unstageAction, &QAction::triggered, this, [this, chunk] {
        applyDiffChunk(chunk, true);
    });
}

void GitEditorWidget::aboutToOpen(const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(realFileName)
    Utils::Id editorId = textDocument()->id();
    if (editorId == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID
            || editorId == Git::Constants::GIT_REBASE_EDITOR_ID) {
        QFileInfo fi(fileName);
        const QString gitPath = fi.absolutePath();
        setSource(gitPath);
        textDocument()->setCodec(
                    GitClient::instance()->encoding(gitPath, "i18n.commitEncoding"));
    }
}

QString GitEditorWidget::decorateVersion(const QString &revision) const
{
    // Format verbose, SHA1 being first token
    return GitClient::instance()->synchronousShortDescription(sourceWorkingDirectory(), revision);
}

QStringList GitEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    QStringList revisions;
    QString errorMessage;
    // Get the SHA1's of the file.
    if (!GitClient::instance()->synchronousParentRevisions(
                sourceWorkingDirectory(), revision, &revisions, &errorMessage)) {
        VcsOutputWindow::appendSilently(errorMessage);
        return QStringList();
    }
    return revisions;
}

bool GitEditorWidget::isValidRevision(const QString &revision) const
{
    return GitClient::instance()->isValidRevision(revision);
}

void GitEditorWidget::addChangeActions(QMenu *menu, const QString &change)
{
    if (contentType() != OtherContent)
        GitClient::addChangeActions(menu, sourceWorkingDirectory(), change);
}

QString GitEditorWidget::revisionSubject(const QTextBlock &inBlock) const
{
    for (QTextBlock block = inBlock.next(); block.isValid(); block = block.next()) {
        const QString line = block.text().trimmed();
        if (line.isEmpty()) {
            block = block.next();
            return block.text().trimmed();
        }
    }
    return QString();
}

bool GitEditorWidget::supportChangeLinks() const
{
    return VcsBaseEditorWidget::supportChangeLinks()
            || (textDocument()->id() == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID)
            || (textDocument()->id() == Git::Constants::GIT_REBASE_EDITOR_ID);
}

QString GitEditorWidget::fileNameForLine(int line) const
{
    // 7971b6e7 share/qtcreator/dumper/dumper.py   (hjk
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    QTC_ASSERT(block.isValid(), return source());
    static QRegularExpression renameExp("^" CHANGE_PATTERN "\\s+([^(]+)");
    const QRegularExpressionMatch match = renameExp.match(block.text());
    if (match.hasMatch()) {
        const QString fileName = match.captured(1).trimmed();
        if (!fileName.isEmpty())
            return fileName;
    }
    return source();
}

QString GitEditorWidget::sourceWorkingDirectory() const
{
    Utils::FilePath path = Utils::FilePath::fromString(source());
    if (!path.isEmpty() && !path.isDir())
        path = path.parentDir();
    while (!path.isEmpty() && !path.exists())
        path = path.parentDir();
    return path.toString();
}

void GitEditorWidget::refresh()
{
    if (VcsBaseEditorConfig *config = editorConfig())
        config->handleArgumentsChanged();
}

QWidget *GitEditorWidget::addFilterWidget()
{
    if (!m_logFilterWidget)
        m_logFilterWidget = new GitLogFilterWidget(this);
    return m_logFilterWidget;
}

QString GitEditorWidget::grepValue() const
{
    if (!m_logFilterWidget)
        return QString();
    return m_logFilterWidget->grepLineEdit->text();
}

QString GitEditorWidget::pickaxeValue() const
{
    if (!m_logFilterWidget)
        return QString();
    return m_logFilterWidget->pickaxeLineEdit->text();
}

bool GitEditorWidget::caseSensitive() const
{
    return m_logFilterWidget && m_logFilterWidget->caseAction->isChecked();
}

} // namespace Internal
} // namespace Git
