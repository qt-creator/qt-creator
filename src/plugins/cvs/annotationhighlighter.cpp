// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"

using namespace Cvs;
using namespace Cvs::Internal;

CvsAnnotationHighlighter::CvsAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                   QTextDocument *document) :
    VcsBase::BaseAnnotationHighlighter(changeNumbers, document)
{ }

QString CvsAnnotationHighlighter::changeNumber(const QString &block) const
{
    const int pos = block.indexOf(QLatin1Char(' '));
    return pos > 1 ? block.left(pos) : QString();
}
