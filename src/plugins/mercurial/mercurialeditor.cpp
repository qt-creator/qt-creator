// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mercurialeditor.h"

#include "annotationhighlighter.h"
#include "constants.h"
#include "mercurialclient.h"
#include "mercurialtr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QString>
#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>

using namespace Utils;

namespace Mercurial::Internal {

// use QRegularExpression::anchoredPattern() when minimum Qt is raised to 5.12+
MercurialEditorWidget::MercurialEditorWidget(MercurialClient *client) :
        exactIdentifier12(QString("\\A(?:") + Constants::CHANGEIDEXACT12 + QString(")\\z")),
        exactIdentifier40(QString("\\A(?:") + Constants::CHANGEIDEXACT40 + QString(")\\z")),
        changesetIdentifier40(Constants::CHANGESETID40),
        m_client(client)
{
    setDiffFilePattern(Constants::DIFFIDENTIFIER);
    setLogEntryPattern("^changeset:\\s+(\\S+)$");
    setAnnotateRevisionTextFormat(Tr::tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(Tr::tr("Annotate &parent revision %1"));
    setAnnotationEntryPattern(Constants::CHANGESETID12);
}

QString MercurialEditorWidget::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        if (exactIdentifier12.match(change).hasMatch())
            return change;
        if (exactIdentifier40.match(change).hasMatch())
            return change;
    }
    return {};
}

VcsBase::BaseAnnotationHighlighter *MercurialEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new MercurialAnnotationHighlighter(changes);
}

QString MercurialEditorWidget::decorateVersion(const QString &revision) const
{
    const FilePath workingDirectory = source().absolutePath();
    // Format with short summary
    return m_client->shortDescriptionSync(workingDirectory, revision);
}

QStringList MercurialEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    const FilePath filePath = source();
    const FilePath workingDirectory = filePath.absolutePath();
    // Retrieve parent revisions
    return m_client->parentRevisionsSync(workingDirectory, filePath.fileName(), revision);
}

} // Mercurial::Internal
