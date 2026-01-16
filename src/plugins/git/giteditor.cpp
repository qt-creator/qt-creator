// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "githighlighters.h"
#include "gitsettings.h"
#include "gittr.h"

#include <coreplugin/icore.h>

#include <texteditor/textdocument.h>

#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsoutputwindow.h>

#include <utils/ansiescapecodehandler.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QMenu>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QToolBar>

#define CHANGE_PATTERN "\\b[a-f0-9]{7,40}\\b"

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

class GitLogFilterWidget : public QToolBar
{
public:
    GitLogFilterWidget(GitEditorWidget *editor)
    {
        auto addLineEdit = [](const QString &placeholder,
                              const QString &tooltip,
                              GitEditorWidget *editor) {
            auto lineEdit = new FancyLineEdit;
            lineEdit->setFiltering(true);
            lineEdit->setToolTip(tooltip);
            lineEdit->setPlaceholderText(placeholder);
            lineEdit->setMaximumWidth(200);
            connect(lineEdit, &QLineEdit::returnPressed,
                    editor, &GitEditorWidget::refresh);
            connect(lineEdit, &FancyLineEdit::rightButtonClicked,
                    editor, &GitEditorWidget::refresh);
            return lineEdit;
        };
        grepLineEdit = addLineEdit(Tr::tr("Filter by message"),
                                   Tr::tr("Filter log entries by text in the commit message."),
                                   editor);
        pickaxeLineEdit = addLineEdit(Tr::tr("Filter by content"),
                                      Tr::tr("Filter log entries by added or removed string."),
                                      editor);
        authorLineEdit = addLineEdit(Tr::tr("Filter by author"),
                                     Tr::tr("Filter log entries by author."),
                                     editor);
        addWidget(new QLabel(Tr::tr("Filter:")));
        addSeparator();
        addWidget(grepLineEdit);
        addSeparator();
        addWidget(pickaxeLineEdit);
        addSeparator();
        addWidget(authorLineEdit);
        addSeparator();
        caseAction = new QAction(Tr::tr("Case Sensitive"), this);
        caseAction->setCheckable(true);
        caseAction->setChecked(true);
        connect(caseAction, &QAction::toggled, editor, &GitEditorWidget::refresh);
        addAction(caseAction);
        hide();
        connect(editor, &GitEditorWidget::toggleFilters, this, &QWidget::setVisible);
    }

    FancyLineEdit *grepLineEdit;
    FancyLineEdit *pickaxeLineEdit;
    FancyLineEdit *authorLineEdit;
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
    setAnnotateRevisionTextFormat(Tr::tr("&Blame %1"));
    setAnnotatePreviousRevisionTextFormat(Tr::tr("Blame &Parent Revision %1"));
    setAnnotationEntryPattern("^(" CHANGE_PATTERN ") ");
}

QString GitEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return {};
    const QString change = cursor.selectedText();
    if (m_changeNumberPattern.match(change).hasMatch())
        return change;
    return {};
}

VcsBase::BaseAnnotationHighlighterCreator GitEditorWidget::annotationHighlighterCreator() const
{
    return VcsBase::getAnnotationHighlighterCreator<GitAnnotationHighlighter>();
}

/**
 * Optionally remove path, author or date specification from annotation, which is tabular:
 * \code
 * 8ca887aa filepath (author YYYY-MM-DD HH:MM:SS <offset> <line>) <content>
 * \endcode
 */
static QString sanitizeBlameOutput(const QString &b)
{
    static const char pattern[] =
        R"(^(\S+)\s(.+?)\s\((.*)\s+(\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}\s\+\d{4}).*?\)(.*)$)";
    static const QRegularExpression re(pattern, QRegularExpression::MultilineOption);

    if (b.isEmpty())
        return b;

    const bool omitPath = settings().omitAnnotationPath();
    const bool omitAuthor = settings().omitAnnotationAuthor();
    const bool omitDate = settings().omitAnnotationDate();

    QString result;
    QRegularExpressionMatchIterator i = re.globalMatch(b);
    while (i.hasNext()) {
        static const QString sep = "  ";
        QRegularExpressionMatch match = i.next();
        const QString hash   = match.captured(1) + sep;
        const QString path   = omitPath   ? QString() : match.captured(2);
        const QString author = omitAuthor ? QString() : match.captured(3) + sep;
        const QString date   = omitDate   ? QString() : match.captured(4);
        const QString code   = match.captured(5);
        result.append(hash + path + "  (" + author + date + ")  " + code + "\n");
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
        AnsiEscapeCodeHandler::setTextInDocument(document(), text);
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

void GitEditorWidget::applyDiffChunk(const DiffChunk& chunk, PatchAction patchAction)
{
    TemporaryFile patchFile("git-apply-chunk");
    if (!patchFile.open())
        return;

    const FilePath baseDir = workingDirectory();
    patchFile.write(chunk.header);
    patchFile.write(chunk.chunk);
    patchFile.close();

    QStringList args = {"--cached"};
    if (patchAction == PatchAction::Revert)
        args << "--reverse";
    QString errorMessage;
    if (gitClient().synchronousApplyPatch(baseDir, patchFile.filePath().path(), &errorMessage, args)) {
        if (errorMessage.isEmpty())
            VcsOutputWindow::appendSilently(baseDir, Tr::tr("Chunk successfully staged"));
        else
            VcsOutputWindow::appendError(baseDir, errorMessage);
        if (patchAction == PatchAction::Revert)
            emit diffChunkReverted();
    } else {
        VcsOutputWindow::appendError(baseDir, errorMessage);
    }
}

void GitEditorWidget::init()
{
    VcsBaseEditorWidget::init();
    Id editorId = textDocument()->id();
    const bool isCommitEditor = editorId == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID;
    const bool isRebaseEditor = editorId == Git::Constants::GIT_REBASE_EDITOR_ID;
    if (!isCommitEditor && !isRebaseEditor)
        return;
    const QChar commentChar = gitClient().commentChar(source());
    if (isCommitEditor)
        textDocument()->resetSyntaxHighlighter(
            [commentChar] { return new GitSubmitHighlighter(commentChar); });
    else if (isRebaseEditor)
        textDocument()->resetSyntaxHighlighter(
            [commentChar] { return new GitRebaseHighlighter(commentChar); });
}

void GitEditorWidget::addDiffActions(QMenu *menu, const DiffChunk &chunk)
{
    menu->addSeparator();

    QAction *stageAction = menu->addAction(Tr::tr("Stage Chunk..."));
    connect(stageAction, &QAction::triggered, this, [this, chunk] {
        applyDiffChunk(chunk, PatchAction::Apply);
    });

    QAction *unstageAction = menu->addAction(Tr::tr("Unstage Chunk..."));
    connect(unstageAction, &QAction::triggered, this, [this, chunk] {
        applyDiffChunk(chunk, PatchAction::Revert);
    });
}

void GitEditorWidget::aboutToOpen(const FilePath &filePath, const FilePath &realFilePath)
{
    Q_UNUSED(realFilePath)
    Id editorId = textDocument()->id();
    if (editorId == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID
            || editorId == Git::Constants::GIT_REBASE_EDITOR_ID) {
        const FilePath gitPath = filePath.absolutePath();
        setSource(gitPath);
        textDocument()->setEncoding(gitClient().encoding(GitClient::EncodingCommit, gitPath));
    }
}

QString GitEditorWidget::decorateVersion(const QString &revision) const
{
    // Format verbose, hash being first token
    return gitClient().synchronousShortDescription(sourceWorkingDirectory(), revision);
}

QStringList GitEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    const Utils::FilePath &repository = sourceWorkingDirectory();
    QStringList revisions;
    QString errorMessage;
    // Get the hashes of the file.
    if (!gitClient().synchronousParentRevisions(repository, revision, &revisions, &errorMessage)) {
        VcsOutputWindow::appendSilently(repository, errorMessage);
        return {};
    }
    return revisions;
}

bool GitEditorWidget::isValidRevision(const QString &revision) const
{
    return gitClient().isValidRevision(revision);
}

void GitEditorWidget::addChangeActions(QMenu *menu, const QString &change)
{
    if (contentType() != OtherContent)
        GitClient::addChangeActions(menu, source(), change);
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
    return {};
}

bool GitEditorWidget::supportChangeLinks() const
{
    return VcsBaseEditorWidget::supportChangeLinks()
            || (textDocument()->id() == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID)
            || (textDocument()->id() == Git::Constants::GIT_REBASE_EDITOR_ID);
}

FilePath GitEditorWidget::fileNameForLine(int line) const
{
    // 7971b6e7 share/qtcreator/dumper/dumper.py   (hjk
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    QTC_ASSERT(block.isValid(), return source());
    static const QRegularExpression renameExp("^" CHANGE_PATTERN "\\s+([^(]+)");
    const QRegularExpressionMatch match = renameExp.match(block.text());
    if (match.hasMatch()) {
        const QString fileName = match.captured(1).trimmed();
        if (!fileName.isEmpty())
            return FilePath::fromString(fileName);
    }
    return source();
}

FilePath GitEditorWidget::sourceWorkingDirectory() const
{
    return GitClient::fileWorkingDirectory(source());
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
        return {};
    return m_logFilterWidget->grepLineEdit->text();
}

QString GitEditorWidget::pickaxeValue() const
{
    if (!m_logFilterWidget)
        return {};
    return m_logFilterWidget->pickaxeLineEdit->text();
}

QString GitEditorWidget::authorValue() const
{
    if (!m_logFilterWidget)
        return {};
    return m_logFilterWidget->authorLineEdit->text();
}

bool GitEditorWidget::caseSensitive() const
{
    return m_logFilterWidget && m_logFilterWidget->caseAction->isChecked();
}

} // Git::Internal
