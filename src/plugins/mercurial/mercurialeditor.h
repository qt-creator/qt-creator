// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Mercurial::Internal {

class MercurialClient;

class MercurialEditorWidget : public VcsBase::VcsBaseEditorWidget
{
public:
    explicit MercurialEditorWidget(MercurialClient *client);

private:
    QString changeUnderCursor(const QTextCursor &cursor) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;
    QString decorateVersion(const QString &revision) const override;
    QStringList annotationPreviousVersions(const QString &revision) const override;

    const QRegularExpression exactIdentifier12;
    const QRegularExpression exactIdentifier40;
    const QRegularExpression changesetIdentifier40;

    MercurialClient *m_client;
};

} // Mercurial::Internal
