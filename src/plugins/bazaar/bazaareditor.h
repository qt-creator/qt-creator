// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Bazaar::Internal {

class BazaarEditorWidget : public VcsBase::VcsBaseEditorWidget
{
public:
    BazaarEditorWidget();

private:
    QString changeUnderCursor(const QTextCursor &cursor) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;

    const QRegularExpression m_changesetId;
    const QRegularExpression m_exactChangesetId;
};

} // Bazaar::Internal
