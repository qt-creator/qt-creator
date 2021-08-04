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
#include <utils/qtcprocess.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QRegularExpression>
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
        m_exactChangesetId(Constants::CHANGESET_ID_EXACT)
    {
        QTC_ASSERT(m_exactChangesetId.isValid(), return);
    }


    const QRegularExpression m_exactChangesetId;
};

FossilEditorWidget::FossilEditorWidget() :
    d(new FossilEditorWidgetPrivate)
{
    setAnnotateRevisionTextFormat(tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate &Parent Revision %1"));
    setDiffFilePattern(Constants::DIFFFILE_ID_EXACT);
    setLogEntryPattern("^.*\\[([0-9a-f]{5,40})\\]");
    setAnnotationEntryPattern(QString("^") + Constants::CHANGESET_ID + " ");
}

FossilEditorWidget::~FossilEditorWidget()
{
    delete d;
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

    const Utils::FilePath workingDirectory = Utils::FilePath::fromString(source()).parentDir();
    const FossilClient *client = FossilPlugin::client();
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
    const Utils::FilePath workingDirectory = Utils::FilePath::fromString(source()).parentDir();
    const FossilClient *client = FossilPlugin::client();
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
