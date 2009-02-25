/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "giteditor.h"

#include "annotationhighlighter.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

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
    if (Git::Constants::debug)
        qDebug() << "GitEditor::GitEditor" << type->type << type->kind;
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
    QString errorMessage;
    // Check for "+++ b/src/plugins/git/giteditor.cpp" (blame and diff)
    // Go back chunks.
    const QString newFileIndicator = QLatin1String("+++ b/");
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(newFileIndicator)) {
            diffFileName.remove(0, newFileIndicator.size());
            const QString fileOrDir = source();
            const QString repo = QFileInfo(fileOrDir).isDir() ?
                GitClient::findRepositoryForDirectory(fileOrDir) : GitClient::findRepositoryForFile(fileOrDir);
            const QString absPath = QDir(repo).absoluteFilePath(diffFileName);
            if (Git::Constants::debug)
                qDebug() << "fileNameFromDiffSpecification" << repo << diffFileName << absPath;
            return absPath;
        }
    }
    return QString();
}

} // namespace Internal
} // namespace Git
