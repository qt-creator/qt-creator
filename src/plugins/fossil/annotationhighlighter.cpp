// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"
#include "constants.h"

#include <utils/qtcassert.h>

namespace Fossil {
namespace Internal {

FossilAnnotationHighlighter::FossilAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                         QTextDocument *document) :
    VcsBase::BaseAnnotationHighlighter(changeNumbers, document),
    m_changesetIdPattern(Constants::CHANGESET_ID)
{
    QTC_CHECK(m_changesetIdPattern.isValid());
}

QString FossilAnnotationHighlighter::changeNumber(const QString &block) const
{
    QRegularExpressionMatch changesetIdMatch = m_changesetIdPattern.match(block);
    if (changesetIdMatch.hasMatch())
        return changesetIdMatch.captured(1);
    return {};
}

} // namespace Internal
} // namespace Fossil
