// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

namespace ClearCase::Internal {

// Annotation highlighter for clearcase triggering on 'changenumber '
class ClearCaseAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
    Q_OBJECT
public:
    explicit ClearCaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                            QTextDocument *document = nullptr);

private:
    QString changeNumber(const QString &block) const override;

    const QChar m_separator = QLatin1Char('|');
};

} // ClearCase::Internal
