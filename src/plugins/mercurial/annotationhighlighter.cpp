// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"
#include "constants.h"

namespace Mercurial::Internal {

MercurialAnnotationHighlighter::MercurialAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                               QTextDocument *document)
    : VcsBase::BaseAnnotationHighlighter(changeNumbers, document),
    changeset(QLatin1String(Constants::CHANGESETID12))
{
}

QString MercurialAnnotationHighlighter::changeNumber(const QString &block) const
{
    const QRegularExpressionMatch match = changeset.match(block);
    if (match.hasMatch())
        return match.captured(1);
    return {};
}

} // Mercurial::Internal
