// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/semantichighlighter.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace ClangCodeModel {
namespace Internal {

class QPropertyHighlighter
{
public:
    QPropertyHighlighter(const QTextDocument *doc, const QString &input, int position);
    ~QPropertyHighlighter();

    const TextEditor::HighlightingResults highlight();

private:
    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace ClangCodeModel

