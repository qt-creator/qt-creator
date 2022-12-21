// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"

using namespace Subversion;
using namespace Subversion::Internal;

SubversionAnnotationHighlighter::SubversionAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                                 QTextDocument *document) :
    VcsBase::BaseAnnotationHighlighter(changeNumbers, document),
    m_blank(QLatin1Char(' '))
{
}

QString SubversionAnnotationHighlighter::changeNumber(const QString &block) const
{
    const int pos = block.indexOf(m_blank);
    return pos > 1 ? block.left(pos) : QString();
}
