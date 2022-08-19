// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

namespace Cvs {
namespace Internal {

// Annotation highlighter for cvs triggering on 'changenumber '
class CvsAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
    Q_OBJECT

public:
    explicit CvsAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                      QTextDocument *document = nullptr);

private:
    QString changeNumber(const QString &block) const override;
};

} // namespace Internal
} // namespace Cvs
