/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitclient.h"
#include "gitsettings.h"
#include <QtCore/QTextCodec>

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QTextStream>

#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>

#define CHANGE_PATTERN_8C "[a-f0-9]{8,8}"
#define CHANGE_PATTERN_40C "[a-f0-9]{40,40}"

namespace Git {
namespace Internal {

// ------------ GitEditor
GitEditor::GitEditor(const VCSBase::VCSBaseEditorParameters *type,
                     QWidget *parent)  :
    VCSBase::VCSBaseEditor(type, parent),
    m_changeNumberPattern8(QLatin1String(CHANGE_PATTERN_8C)),
    m_changeNumberPattern40(QLatin1String(CHANGE_PATTERN_40C))
{
    QTC_ASSERT(m_changeNumberPattern8.isValid(), return);
    QTC_ASSERT(m_changeNumberPattern40.isValid(), return);
    setAnnotateRevisionTextFormat(tr("Blame %1"));
    if (Git::Constants::debug)
        qDebug() << "GitEditor::GitEditor" << type->type << type->id;
}

QSet<QString> GitEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r(QLatin1String("^("CHANGE_PATTERN_8C") "));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n("CHANGE_PATTERN_8C") "));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    if (Git::Constants::debug)
        qDebug() << "GitEditor::annotationChanges() returns #" << changes.size();
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
    if (Git::Constants::debug > 1)
        qDebug() << "GitEditor:::changeUnderCursor:" << change;
    if (m_changeNumberPattern8.exactMatch(change))
        return change;
    if (m_changeNumberPattern40.exactMatch(change))
        return change;
    return QString();
}

VCSBase::DiffHighlighter *GitEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^[-+][-+][-+] [ab].*"));
    return new VCSBase::DiffHighlighter(filePattern);
}

VCSBase::BaseAnnotationHighlighter *GitEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new GitAnnotationHighlighter(changes);
}

QString GitEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
        // Check for "+++ b/src/plugins/git/giteditor.cpp" (blame and diff)
    // Go back chunks.
    const QString newFileIndicator = QLatin1String("+++ b/");
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(newFileIndicator)) {
            diffFileName.remove(0, newFileIndicator.size());
            return findDiffFile(diffFileName, GitPlugin::instance()->versionControl());
        }
    }
    return QString();
}

/* Remove the date specification from annotation, which is tabular:
\code
8ca887aa (author               YYYY-MM-DD HH:MM:SS <offset>  <line>)<content>
\endcode */

static void removeAnnotationDate(QString *s)
{
    if (s->isEmpty())
        return;
    // Get position of date (including blank) and the ')'
    const QRegExp isoDatePattern(QLatin1String(" \\d{4}-\\d{2}-\\d{2}"));
    Q_ASSERT(isoDatePattern.isValid());
    const int datePos = s->indexOf(isoDatePattern);
    const int parenPos = datePos == -1 ? -1 : s->indexOf(QLatin1Char(')'));
    if (parenPos == -1)
        return;
    // In all lines, remove the bit from datePos .. parenPos;
    const int dateLength = parenPos - datePos;
    const QChar newLine = QLatin1Char('\n');
    for (int pos = 0; pos < s->size(); ) {
        if (pos + parenPos >s->size()) // Should not happen
            break;
        s->remove(pos + datePos, dateLength);
        const int nextLinePos = s->indexOf(newLine, pos + datePos);
        pos = nextLinePos == -1 ? s->size() : nextLinePos  + 1;
    }
}

void GitEditor::setPlainTextDataFiltered(const QByteArray &a)
{
    // If desired, filter out the date from annotation
    const bool omitAnnotationDate = contentType() == VCSBase::AnnotateOutput
                                    && GitPlugin::instance()->settings().omitAnnotationDate;
    if (omitAnnotationDate) {
        QString text = codec()->toUnicode(a);
        removeAnnotationDate(&text);
        setPlainText(text);
    } else {
        setPlainTextData(a);
    }
}

void GitEditor::commandFinishedGotoLine(bool ok, const QVariant &v)
{
    if (ok && v.type() == QVariant::Int) {
        const int line = v.toInt();
        if (line >= 0)
            gotoLine(line);
    }
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
        VCSBase::VCSBaseOutputWindow::instance()->appendSilently(errorMessage);
        return QStringList();
    }
    // Format verbose, SHA1 being first token
    QStringList descriptions;
    if (!client->synchronousShortDescriptions(workingDirectory, revisions, &descriptions, &errorMessage)) {
        VCSBase::VCSBaseOutputWindow::instance()->appendSilently(errorMessage);
        return QStringList();
    }
    return descriptions;
}

} // namespace Internal
} // namespace Git

