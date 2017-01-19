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
#include "gitplugin.h"
#include "gitclient.h"
#include "gitsettings.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "githighlighters.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <vcsbase/vcsoutputwindow.h>
#include <texteditor/textdocument.h>

#include <utils/temporaryfile.h>

#include <QMenu>

#include <QFileInfo>
#include <QRegExp>
#include <QSet>
#include <QTextCodec>
#include <QDir>

#include <QTextCursor>
#include <QTextBlock>
#include <QMessageBox>

#define CHANGE_PATTERN "[a-f0-9]{7,40}"

using namespace VcsBase;

namespace Git {
namespace Internal {

GitEditorWidget::GitEditorWidget() :
    m_changeNumberPattern(CHANGE_PATTERN)
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), return);
    /* Diff format:
        diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp
        index 40997ff..4e49337 100644
        --- a/src/plugins/git/giteditor.cpp
        +++ b/src/plugins/git/giteditor.cpp
    */
    setDiffFilePattern(QRegExp("^(?:diff --git a/|index |[+-]{3} (?:/dev/null|[ab]/(.+$)))"));
    setLogEntryPattern(QRegExp("^commit ([0-9a-f]{8})[0-9a-f]{32}"));
    setAnnotateRevisionTextFormat(tr("&Blame %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Blame &Parent Revision %1"));
}

QSet<QString> GitEditorWidget::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r("^(" CHANGE_PATTERN ") ");
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern("\n(" CHANGE_PATTERN ") ");
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    return changes;
}

QString GitEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    if (m_changeNumberPattern.exactMatch(change))
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

    const bool omitDate = GitPlugin::client()->settings().boolValue(
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
        result.append(b.mid(prevPos, stripPos));
        result.append(b.mid(afterParen, pos - afterParen));
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
    case AnnotateOutput:
        modText = sanitizeBlameOutput(text);
        break;
    default:
        break;
    }

    textDocument()->setPlainText(modText);
}

void GitEditorWidget::checkoutChange()
{
    GitPlugin::client()->stashAndCheckout(sourceWorkingDirectory(), m_currentChange);
}

void GitEditorWidget::resetChange(const QByteArray &resetType)
{
    GitPlugin::client()->reset(
                sourceWorkingDirectory(), QLatin1String("--" + resetType), m_currentChange);
}

void GitEditorWidget::cherryPickChange()
{
    GitPlugin::client()->synchronousCherryPick(sourceWorkingDirectory(), m_currentChange);
}

void GitEditorWidget::revertChange()
{
    GitPlugin::client()->synchronousRevert(sourceWorkingDirectory(), m_currentChange);
}

void GitEditorWidget::logChange()
{
    GitPlugin::client()->log(
                sourceWorkingDirectory(), QString(), false, { m_currentChange });
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

    QStringList args = { "--cached" };
    if (revert)
        args << "--reverse";
    QString errorMessage;
    if (GitPlugin::client()->synchronousApplyPatch(baseDir, patchFile.fileName(), &errorMessage, args)) {
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
    Core::Id editorId = textDocument()->id();
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
    Core::Id editorId = textDocument()->id();
    if (editorId == Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID
            || editorId == Git::Constants::GIT_REBASE_EDITOR_ID) {
        QFileInfo fi(fileName);
        const QString gitPath = fi.absolutePath();
        setSource(gitPath);
        textDocument()->setCodec(
                    GitPlugin::client()->encoding(gitPath, "i18n.commitEncoding"));
    }
}

QString GitEditorWidget::decorateVersion(const QString &revision) const
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();

    // Format verbose, SHA1 being first token
    return GitPlugin::client()->synchronousShortDescription(workingDirectory, revision);
}

QStringList GitEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    QStringList revisions;
    QString errorMessage;
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Get the SHA1's of the file.
    if (!GitPlugin::client()->synchronousParentRevisions(workingDirectory, revision, &revisions, &errorMessage)) {
        VcsOutputWindow::appendSilently(errorMessage);
        return QStringList();
    }
    return revisions;
}

bool GitEditorWidget::isValidRevision(const QString &revision) const
{
    return GitPlugin::client()->isValidRevision(revision);
}

void GitEditorWidget::addChangeActions(QMenu *menu, const QString &change)
{
    m_currentChange = change;
    if (contentType() != OtherContent) {
        connect(menu->addAction(tr("Cherr&y-Pick Change %1").arg(change)), &QAction::triggered,
                this, &GitEditorWidget::cherryPickChange);
        connect(menu->addAction(tr("Re&vert Change %1").arg(change)), &QAction::triggered,
                this, &GitEditorWidget::revertChange);
        connect(menu->addAction(tr("C&heckout Change %1").arg(change)), &QAction::triggered,
                this, &GitEditorWidget::checkoutChange);
        connect(menu->addAction(tr("&Log for Change %1").arg(change)), &QAction::triggered,
                this, &GitEditorWidget::logChange);

        QMenu *resetMenu = new QMenu(tr("&Reset to Change %1").arg(change), menu);
        connect(resetMenu->addAction(tr("&Hard")), &QAction::triggered,
                this, [this]() { resetChange("hard"); });
        connect(resetMenu->addAction(tr("&Mixed")), &QAction::triggered,
                this, [this]() { resetChange("mixed"); });
        connect(resetMenu->addAction(tr("&Soft")), &QAction::triggered,
                this, [this]() { resetChange("soft"); });
        menu->addMenu(resetMenu);
    }
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
    static QRegExp renameExp("^" CHANGE_PATTERN "\\s+([^(]+)");
    if (renameExp.indexIn(block.text()) != -1) {
        const QString fileName = renameExp.cap(1).trimmed();
        if (!fileName.isEmpty())
            return fileName;
    }
    return source();
}

QString GitEditorWidget::sourceWorkingDirectory() const
{
    const QFileInfo fi(source());
    return fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
}

} // namespace Internal
} // namespace Git
