// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossileditor.h"

#include "annotationhighlighter.h"
#include "constants.h"
#include "fossilclient.h"
#include "fossilplugin.h"
#include "fossiltr.h"

#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QTextCursor>

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
    setAnnotateRevisionTextFormat(Tr::tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(Tr::tr("Annotate &Parent Revision %1"));
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
        const QRegularExpressionMatch exactChangesetIdMatch = d->m_exactChangesetId.match(change);
        if (exactChangesetIdMatch.hasMatch())
            return change;
    }
    return {};
}

QString FossilEditorWidget::decorateVersion(const QString &revision) const
{
    static const int shortChangesetIdSize(10);
    static const int maxTextSize(120);

    const Utils::FilePath workingDirectory = source().parentDir();
    const FossilClient *client = FossilPlugin::client();
    const RevisionInfo revisionInfo = client->synchronousRevisionQuery(workingDirectory, revision,
                                                                       true);
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
    const Utils::FilePath workingDirectory = source().parentDir();
    const FossilClient *client = FossilPlugin::client();
    const RevisionInfo revisionInfo = client->synchronousRevisionQuery(workingDirectory, revision);
    if (revisionInfo.parentId.isEmpty())
        return {};

    QStringList revisions{revisionInfo.parentId};
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
