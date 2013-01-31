/****************************************************************************
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

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitclient.h"
#include "gitsettings.h"
#include <QTextCodec>

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QSet>
#include <QTextStream>

#include <QTextCursor>
#include <QTextEdit>
#include <QTextBlock>

#define CHANGE_PATTERN_8C "[a-f0-9]{7,8}"
#define CHANGE_PATTERN_40C "[a-f0-9]{40,40}"

namespace Git {
namespace Internal {

// ------------ GitEditor
GitEditor::GitEditor(const VcsBase::VcsBaseEditorParameters *type,
                     QWidget *parent)  :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_changeNumberPattern8(QLatin1String(CHANGE_PATTERN_8C)),
    m_changeNumberPattern40(QLatin1String(CHANGE_PATTERN_40C))
{
    QTC_ASSERT(m_changeNumberPattern8.isValid(), return);
    QTC_ASSERT(m_changeNumberPattern40.isValid(), return);
    /* Diff format:
        diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp
        index 40997ff..4e49337 100644
        --- a/src/plugins/git/giteditor.cpp
        +++ b/src/plugins/git/giteditor.cpp
    */
    setDiffFilePattern(QRegExp(QLatin1String("^(?:diff --git a/|index |[+-]{3} (?:/dev/null|[ab]/(.+$)))")));
    setLogEntryPattern(QRegExp(QLatin1String("^commit ([0-9a-f]{8})[0-9a-f]{32}")));
    setAnnotateRevisionTextFormat(tr("Blame %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Blame Parent Revision %1"));
}

QSet<QString> GitEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r(QLatin1String("^(" CHANGE_PATTERN_8C ") "));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n(" CHANGE_PATTERN_8C ") "));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    return changes;
}

QString GitEditor::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    const QString change = cursor.selectedText();
    if (m_changeNumberPattern8.exactMatch(change))
        return change;
    if (m_changeNumberPattern40.exactMatch(change))
        return change;
    return QString();
}

VcsBase::BaseAnnotationHighlighter *GitEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                           const QColor &bg) const
{
    return new GitAnnotationHighlighter(changes, bg);
}

/* Remove the date specification from annotation, which is tabular:
\code
8ca887aa (author               YYYY-MM-DD HH:MM:SS <offset>  <line>)<content>
\endcode */

static QByteArray removeAnnotationDate(const QByteArray &b)
{
    if (b.isEmpty())
        return QByteArray();

    const int parenPos = b.indexOf(')');
    if (parenPos == -1)
        return QByteArray(b);
    int datePos = parenPos;

    int i = parenPos;
    while (i >= 0 && b.at(i) != ' ')
        --i;
    while (i >= 0 && b.at(i) == ' ')
        --i;
    int spaceCount = 0;
    // i is now on timezone. Go back 3 spaces: That is where the date starts.
    while (i >= 0) {
        if (b.at(i) == ' ')
            ++spaceCount;
        if (spaceCount == 3) {
            datePos = i;
            break;
        }
        --i;
    }
    if (datePos == 0)
        return QByteArray(b);

    // Copy over the parts that have not changed into a new byte array
    QByteArray result;
    QTC_ASSERT(b.size() >= parenPos, return result);
    int prevPos = 0;
    int pos = b.indexOf('\n', 0) + 1;
    forever {
        QTC_CHECK(prevPos < pos);
        int afterParen = prevPos + parenPos;
        result.append(b.constData() + prevPos, datePos);
        result.append(b.constData() + afterParen, pos - afterParen);
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

void GitEditor::setPlainTextDataFiltered(const QByteArray &a)
{
    QByteArray array = a;
    GitPlugin *plugin = GitPlugin::instance();
    // If desired, filter out the date from annotation
    switch (contentType())
    {
    case VcsBase::AnnotateOutput: {
        const bool omitAnnotationDate = plugin->settings().boolValue(GitSettings::omitAnnotationDateKey);
        if (omitAnnotationDate)
            array = removeAnnotationDate(a);
        break;
    }
    case VcsBase::DiffOutput: {
        const QFileInfo fi(source());
        const QString workingDirectory = fi.absolutePath();
        QByteArray precedes, follows;
        if (array.startsWith("commit ")) { // show
            int lastHeaderLine = array.indexOf("\n\n") + 1;
            plugin->gitClient()->synchronousTagsForCommit(workingDirectory, QLatin1String(array.mid(7, 8)), precedes, follows);
            if (!precedes.isEmpty())
                array.insert(lastHeaderLine, "Precedes: " + precedes + '\n');
            if (!follows.isEmpty())
                array.insert(lastHeaderLine, "Follows: " + follows + '\n');
        }
        break;
    }
    default:
        break;
    }

    setPlainTextData(array);
}

void GitEditor::commandFinishedGotoLine(bool ok, int /* exitCode */, const QVariant &v)
{
    if (ok && v.type() == QVariant::Int) {
        const int line = v.toInt();
        if (line >= 0)
            gotoLine(line);
    }
}

void GitEditor::cherryPickChange()
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    GitPlugin::instance()->gitClient()->cherryPickCommit(workingDirectory, m_currentChange);
}

void GitEditor::revertChange()
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    GitPlugin::instance()->gitClient()->revertCommit(workingDirectory, m_currentChange);
}

QString GitEditor::decorateVersion(const QString &revision) const
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();

    // Format verbose, SHA1 being first token
    return GitPlugin::instance()->gitClient()->synchronousShortDescription(workingDirectory, revision);
}

QStringList GitEditor::annotationPreviousVersions(const QString &revision) const
{
    QStringList revisions;
    QString errorMessage;
    GitClient *client = GitPlugin::instance()->gitClient();
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Get the SHA1's of the file.
    if (!client->synchronousParentRevisions(workingDirectory, QStringList(fi.fileName()),
                                            revision, &revisions, &errorMessage)) {
        VcsBase::VcsBaseOutputWindow::instance()->appendSilently(errorMessage);
        return QStringList();
    }
    return revisions;
}

bool GitEditor::isValidRevision(const QString &revision) const
{
    return GitPlugin::instance()->gitClient()->isValidRevision(revision);
}

void GitEditor::addChangeActions(QMenu *menu, const QString &change)
{
    m_currentChange = change;
    menu->addAction(tr("Cherry-pick Change %1").arg(change), this, SLOT(cherryPickChange()));
    menu->addAction(tr("Revert Change %1").arg(change), this, SLOT(revertChange()));
}

QString GitEditor::revisionSubject(const QTextBlock &inBlock) const
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

} // namespace Internal
} // namespace Git
