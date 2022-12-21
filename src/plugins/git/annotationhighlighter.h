// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

namespace Git {
namespace Internal {

// Annotation highlighter for p4 triggering on 'changenumber:'
class GitAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
    Q_OBJECT
public:
    explicit GitAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                      QTextDocument *document = nullptr);

private:
    QString changeNumber(const QString &block) const override;

    const QChar m_blank = ' ';
};

} // namespace Internal
} // namespace Git
