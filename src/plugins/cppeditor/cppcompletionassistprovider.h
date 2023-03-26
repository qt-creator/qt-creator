// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/codeassist/assistenums.h>
#include <texteditor/codeassist/completionassistprovider.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace CPlusPlus { struct LanguageFeatures; }

namespace TextEditor {
class TextEditorWidget;
class AssistInterface;
}

namespace Utils { class FilePath; }

namespace CppEditor {

class CPPEDITOR_EXPORT CppCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    CppCompletionAssistProvider(QObject *parent = nullptr);
    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    virtual std::unique_ptr<TextEditor::AssistInterface> createAssistInterface(
        const Utils::FilePath &filePath,
        const TextEditor::TextEditorWidget *textEditorWidget,
        const CPlusPlus::LanguageFeatures &languageFeatures,
        TextEditor::AssistReason reason) const = 0;

    static int activationSequenceChar(const QChar &ch, const QChar &ch2,
                                      const QChar &ch3, unsigned *kind,
                                      bool wantFunctionCall, bool wantQt5SignalSlots);
};

} // namespace CppEditor
