// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Bazaar {
namespace Internal {

class BazaarEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    BazaarEditorWidget();

private:
    QString changeUnderCursor(const QTextCursor &cursor) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;

    const QRegularExpression m_changesetId;
    const QRegularExpression m_exactChangesetId;
};

} // namespace Internal
} // namespace Bazaar
