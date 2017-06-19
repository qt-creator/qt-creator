/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"

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

namespace CppTools {

class CPPTOOLS_EXPORT CppCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    CppCompletionAssistProvider(QObject *parent = 0);
    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    virtual TextEditor::AssistInterface *createAssistInterface(
            const QString &filePath,
            const TextEditor::TextEditorWidget *textEditorWidget,
            const CPlusPlus::LanguageFeatures &languageFeatures,
            int position,
            TextEditor::AssistReason reason) const = 0;

    static int activationSequenceChar(const QChar &ch, const QChar &ch2,
                                      const QChar &ch3, unsigned *kind,
                                      bool wantFunctionCall, bool wantQt5SignalSlots);
};

} // namespace CppTools
