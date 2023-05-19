// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"

namespace ClearCase::Internal {

ClearCaseAnnotationHighlighter::ClearCaseAnnotationHighlighter(const VcsBase::Annotation &annotation,
                                                               QTextDocument *document)
    : VcsBase::BaseAnnotationHighlighter(annotation, document)
{}

QString ClearCaseAnnotationHighlighter::changeNumber(const QString &block) const
{
    const int pos = block.indexOf(m_separator);
    return pos > 1 ? block.left(pos) : QString();
}

} // namespace ClearCase::Internal
