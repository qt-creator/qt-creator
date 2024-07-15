// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossileditor.h"

#include "annotationhighlighter.h"
#include "constants.h"
#include "fossilclient.h"
#include "fossiltr.h"

#include <utils/qtcassert.h>

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>
#include <QTextCursor>

namespace Fossil::Internal {

class FossilEditorWidget final : public VcsBase::VcsBaseEditorWidget
{
public:
    FossilEditorWidget()
        : m_exactChangesetId(Constants::CHANGESET_ID_EXACT)
    {
        QTC_CHECK(m_exactChangesetId.isValid());
        setAnnotateRevisionTextFormat(Tr::tr("&Annotate %1"));
        setAnnotatePreviousRevisionTextFormat(Tr::tr("Annotate &Parent Revision %1"));
        setDiffFilePattern(Constants::DIFFFILE_ID_EXACT);
        setLogEntryPattern("^.*\\[([0-9a-f]{5,40})\\]");
        setAnnotationEntryPattern(QString("^") + Constants::CHANGESET_ID + " ");
    }

private:
    QString changeUnderCursor(const QTextCursor &cursor) const final;
    QString decorateVersion(const QString &revision) const final;
    QStringList annotationPreviousVersions(const QString &revision) const final;
    VcsBase::BaseAnnotationHighlighterCreator annotationHighlighterCreator() const final;

    const QRegularExpression m_exactChangesetId;
};

QString FossilEditorWidget::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        const QRegularExpressionMatch exactChangesetIdMatch = m_exactChangesetId.match(change);
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
    const RevisionInfo revisionInfo =
        fossilClient().synchronousRevisionQuery(workingDirectory, revision, true);
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
    const RevisionInfo revisionInfo =
        fossilClient().synchronousRevisionQuery(workingDirectory, revision);
    if (revisionInfo.parentId.isEmpty())
        return {};

    QStringList revisions{revisionInfo.parentId};
    revisions.append(revisionInfo.mergeParentIds);
    return revisions;
}

VcsBase::BaseAnnotationHighlighterCreator FossilEditorWidget::annotationHighlighterCreator() const
{
    return VcsBase::getAnnotationHighlighterCreator<FossilAnnotationHighlighter>();
}

QWidget *createFossilEditorWidget()
{
    return new FossilEditorWidget;
}

} // namespace Fossil::Internal
