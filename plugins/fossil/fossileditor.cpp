/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include "fossileditor.h"
#include "annotationhighlighter.h"
#include "constants.h"
#include "fossilplugin.h"
#include "fossilclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QRegularExpression>
#include <QRegExp>
#include <QString>
#include <QTextCursor>
#include <QTextBlock>
#include <QDir>
#include <QFileInfo>

namespace Fossil {
namespace Internal {

class FossilEditorWidgetPrivate
{
public:
    FossilEditorWidgetPrivate() :
        m_exactChangesetId(Constants::CHANGESET_ID_EXACT),
        m_firstChangesetId(QString("\n") + Constants::CHANGESET_ID + " "),
        m_nextChangesetId(m_firstChangesetId)
    {
        QTC_ASSERT(m_exactChangesetId.isValid(), return);
        QTC_ASSERT(m_firstChangesetId.isValid(), return);
        QTC_ASSERT(m_nextChangesetId.isValid(), return);
    }


    const QRegularExpression m_exactChangesetId;
    const QRegularExpression m_firstChangesetId;
    const QRegularExpression m_nextChangesetId;
};

FossilEditorWidget::FossilEditorWidget() :
    d(new FossilEditorWidgetPrivate)
{
    setAnnotateRevisionTextFormat(tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate &Parent Revision %1"));

    const QRegExp exactDiffFileIdPattern(Constants::DIFFFILE_ID_EXACT);
    QTC_ASSERT(exactDiffFileIdPattern.isValid(), return);
    setDiffFilePattern(exactDiffFileIdPattern);

    const QRegExp logChangePattern("^.*\\[([0-9a-f]{5,40})\\]");
    QTC_ASSERT(logChangePattern.isValid(), return);
    setLogEntryPattern(logChangePattern);
}

FossilEditorWidget::~FossilEditorWidget()
{
    delete d;
}

QSet<QString> FossilEditorWidget::annotationChanges() const
{

    const QString txt = toPlainText();
    if (txt.isEmpty())
        return QSet<QString>();

    // extract changeset id at the beginning of each annotated line:
    // <changeid> ...:

    QSet<QString> changes;

    QRegularExpressionMatch firstChangesetIdMatch = d->m_firstChangesetId.match(txt);
    if (firstChangesetIdMatch.hasMatch()) {
        QString changeId = firstChangesetIdMatch.captured(1);
        changes.insert(changeId);

        QRegularExpressionMatchIterator i = d->m_nextChangesetId.globalMatch(txt);
        while (i.hasNext()) {
            const QRegularExpressionMatch nextChangesetIdMatch = i.next();
            changeId = nextChangesetIdMatch.captured(1);
            changes.insert(changeId);
        }
    }
    return changes;
}

QString FossilEditorWidget::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        QRegularExpressionMatch exactChangesetIdMatch = d->m_exactChangesetId.match(change);
        if (exactChangesetIdMatch.hasMatch())
            return change;
    }
    return QString();
}

QString FossilEditorWidget::decorateVersion(const QString &revision) const
{
    static const int shortChangesetIdSize(10);
    static const int maxTextSize(120);

    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    FossilClient *client = FossilPlugin::instance()->client();
    RevisionInfo revisionInfo =
            client->synchronousRevisionQuery(workingDirectory, revision, true);

    // format: 'revision (committer "comment...")'
    QString output = revision.left(shortChangesetIdSize)
            + " (" + revisionInfo.committer
            + " \"" + revisionInfo.commentMsg.left(maxTextSize);

    if (output.size() > maxTextSize) {
        output.truncate(maxTextSize - 3);
        output.append("...");
    }
    output.append("\")");
    return output;
}

QStringList FossilEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    QStringList revisions;
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    FossilClient *client = FossilPlugin::instance()->client();
    RevisionInfo revisionInfo =
            client->synchronousRevisionQuery(workingDirectory, revision);
    if (revisionInfo.parentId.isEmpty())
        return QStringList();

    revisions.append(revisionInfo.parentId);
    revisions.append(revisionInfo.mergeParentIds);
    return revisions;
}

VcsBase::BaseAnnotationHighlighter *FossilEditorWidget::createAnnotationHighlighter(
        const QSet<QString> &changes) const
{
    return new FossilAnnotationHighlighter(changes);
}

} // namespace Internal
} // namespace Fossil
