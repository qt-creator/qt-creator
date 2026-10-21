// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor.h"

#include <QSet>

namespace TextEditor::Internal {

class JsonEditorWidget final : public TextEditorWidget
{
public:
    JsonEditorWidget();

protected:
    int scopeStartPosition(const QTextBlock &endBlock, int bracePosition) const override;
    QString scopeLabelText(const QTextBlock &openBlock, int openBracePosition) const override;

private:
    int findScopeStartPosition(const QTextBlock &endBlock, int bracePosition) const;
    QString findScopeLabelText(const QTextBlock &openBlock, int openBracePosition) const;
    void indexArrayElements(int arrayPosition) const;

    // By the position of the closing brace or bracket, the position scopeStartPosition()
    // returned.
    mutable QHash<int, int> m_scopeStartCache;
    // By the position of the opening brace or bracket, the label scopeLabelText() returned.
    mutable QHash<int, QString> m_scopeLabelCache;
    // By the position of its first token, the index of an element in its array.
    mutable QHash<int, int> m_elementIndexCache;
    // The positions of the arrays whose elements are in m_elementIndexCache.
    mutable QSet<int> m_indexedArrays;
};

void setupJsonEditor();

} // TextEditor::Internal
