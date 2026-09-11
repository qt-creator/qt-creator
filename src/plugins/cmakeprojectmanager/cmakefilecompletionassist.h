// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/completionassistprovider.h>

#include <QObject>

namespace CMakeProjectManager::Internal {

class CMakeFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const final;
    int activationCharSequenceLength() const final;
    bool isActivationCharSequence(const QString &sequence) const final;
};

// What the signature of a call is, for whoever asks for that alone.  The
// completion of a command that takes keywords offers those instead, since
// they are what is being written.
class CMakeFunctionHintAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const final;
    int activationCharSequenceLength() const final;
    bool isActivationCharSequence(const QString &sequence) const final;
};

// The one of those, which the document of a CMake file asks for the
// signature of a call with.
TextEditor::CompletionAssistProvider &cmakeFunctionHintAssistProvider();

#ifdef WITH_TESTS
QObject *createCMakeFunctionHintTest();
#endif

} // CMakeProjectManager::Internal
